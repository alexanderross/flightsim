require 'rack/app'
require 'slim'
require 'tilt'
require 'timeout' #So what, I used timeout :D
require 'yaml'
require 'json'

RFCOMM_CMD_PATH = "/tmp/rfcmdpath"
PITCH_STATE_PATH = "/tmp/pitch_state"
ROLL_STATE_PATH = "/tmp/roll_state"

CMD_JOIN_CHR= "$"

#The registers and values needed to ensure a drive is properly set up for the sim. Applies to both axes. One would argue this isn't necessary,
# as if the drive is not configured it will not be able to communicate with the axis controller. But they just haters. 

DRIVE_ATTRS = YAML.load(File.read("drive_attrs.yml"))

SETUP_SEQUENCE = [
  [67, 3],
  [66, 3],
  [64, 1],
  [20, 1500],
  [51, 1500]
]

File.mkfifo(RFCOMM_CMD_PATH) unless File.exist?(RFCOMM_CMD_PATH)

class AxisCommand
    def initialize(axis, dest, val)
        @axis = axis
        @dest = dest
        @val = val
    end

    def to_s
      preamble = "W"+(@axis[0].downcase[0] == 'p' ? "I" : "O")
      preamble + sprintf("%03d", @dest.to_i) + sprintf("%05d", @val.to_i)
    end
end

class DriveStateAttr
    def initialize(num, position, data)
      @data = data
      @val = self.class.parse_value_from(num, position)
      @name = data['label']
      @label = @val ? data['on_label'] : data['off_label']
    end

    def self.parse_value_from(num, position)
      ((1 << position.to_i) & num) > 0
    end

    def to_h
      {
        :name => @name,
        :label => @label,
        :val => @val
      }
    end
end


class App < Rack::App

  desc 'Write a value to a register for a drive'
  validate_params do
    required 'axis', :class => String, :desc => "Axis to write to ('roll' or 'pitch')", :example => 'roll'
    required 'r', :class => String, :desc => 'The register to write to', :example => '177'
    required 'v', :class => String, :desc => 'The value to write', :example => '1200'
  end

  get '/write' do
    axis = process_axis(params)
    msg, success = write_to_drive(AxisCommand.new(axis, params['r'], params['v']))
    response.status = 500 unless success

    msg

  end

  get '/write_dual' do
    rolldest, rollval = params["rc"].split("|")
    roll_cmd = AxisCommand.new("roll", rolldest, rollval)

    pitchdest, pitchval = params["pc"].split("|")
    pitch_cmd = AxisCommand.new("pitch", pitchdest, pitchval)

    msg, success = write_to_drive([roll_cmd, pitch_cmd])
    response.status = 500 unless success

    msg
  end

  get '/states.json' do 
    read_drive_states.to_json
  end

  get '/' do
    Tilt.new("views/index.html.slim").render;
  end

  post '/setup' do 
    output = ""
    SETUP_SEQUENCE.each do |itm|
        axis = process_axis(params)
        write_to_drive(process_axis(params), itm[0], itm[1])
        output << "Wrote #{itm[1]} to #{itm[0]} on axis '#{axis}'"
    end

    output
  end

  def process_axis(params)
    params['axis'].downcase[0]
  end

  def read_drive_states()
    roll = File.read(ROLL_STATE_PATH).to_i rescue nil
    pitch = File.read(PITCH_STATE_PATH).to_i rescue nil
    return {pitch: parse_drive_state(pitch), roll: parse_drive_state(roll)}
  end

  def parse_drive_state(axis_num)
    return nil if axis_num.nil?

    [].tap do |ary|
      DRIVE_ATTRS.each do |pos, data|
        ary << DriveStateAttr.new(axis_num, pos, data).to_h
      end 
    end
  end

  def write_to_drive(commands)
    #Fine we'll use a damn file
    begin
      #(W)rite to P(i)tch or R(o)ll
      msg = Array(commands).join(CMD_JOIN_CHR)

      #This gets passed straight through the RF interface.
      #W[Axis(1)][Register(3)][Value(5)] for 10b total.

      #10ms timeout
      Timeout::timeout(1/100.0) {
        File.open(RFCOMM_CMD_PATH, 'w') do |f| 
            f.write(msg+"\0")
        end
      }

      return ["Wrote #{msg} to rf", true]
    rescue TimeoutError => e
      return ["Write timed out - RFCOMM program likely not up or is slow AF.", false]
    rescue StandardError => e
      return [e, false]
    end
  end

end