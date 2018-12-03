require 'rack/app'

RFCOMM_CMD_PATH = "/tmp/rfcmdpath"

#The registers and values needed to ensure a drive is properly set up for the sim. Applies to both axes. One would argue this isn't necessary,
# as if the drive is not configured it will not be able to communicate with the axis controller. But they just haters. 
SETUP_SEQUENCE = [
  [67, 3],
  [66, 3],
  [64, 1],
  [20, 1500],
  [51, 1500]
]


class App < Rack::App

  desc 'Write a value to a register for a drive'
  validate_params do
    required 'axis', :class => String, :desc => "Axis to write to ('roll' or 'pitch')", :example => 'roll'
    required 'r', :class => String, :desc => 'The register to write to', :example => '177'
    required 'v', :class => String, :desc => 'The value to write', :example => '1200'
  end

  post '/write' do
    axis = process_axis(params)
    write_to_drive(axis, params['r'], params['v'])
  end

  get '/' do
    "I'm Alright"
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

  def write_to_drive(axis, register, value)
    #Fine we'll use a damn file
    begin
      #(W)rite to P(i)tch or R(o)ll
      preamble = "W"+(axis[0] == 'p' ? "I" : "O")

      #This gets passed straight through the RF interface.
      #W[Axis(1)][Register(3)][Value(5)] for 10b total.
      File.open(RFCOMM_CMD_PATH, 'w') { |file| file.write(preamble + sprintf("%03d", register.to_i) + sprintf("%05d", value.to_i)) }
      return "Wrote #{value} to #{register} on #{axis}"
    rescue StandardError => e
      return e.message
    end
  end

end