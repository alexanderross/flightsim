require 'rubyserial'
require 'timeout'


class SerialInterfaceUtil

  CMD_DESC = {
    read: "Read the contents of a Pxxx register eg. 'read 177' ",
    write: "Write hex value to register eg. 'write 177 100'",
    moveto: "Move drive to indicated position eg. 'moveto 170'",
    exit: "Quit"
  }

  CONN_TIMEOUT = 5 #seconds

  attr_reader :serconn

  class << self
    def start(args)
      self.new(port: args.first || '/dev/ttyACM0')
    end
  end


  def initialize(port:, baud: 38400, data_bits: 8, parity: :even)
    @serconn = Serial.new(port, baud, data_bits, parity)

    run
  end

  def run
    while input = get_input
      
    end
  end

  private

  def get_input
    puts "Enter command:\n"
    puts ">"
    params = gets.chomp.split(" ")
    cmd = (params[0] || '').to_sym

    if(CMD_DESC.keys.include?(cmd))
      eval("proc_#{cmd}('#{params[1..-1].map(&:to_i).join('\',\'')}')")
    else
      puts "Unknown command. Type 'help' to display available commands."
    end

  end

  def send_to_serial(msg_obj)
    @sconn.write(msg_obj)
  rescue RubySerial::Error => e
    puts "Serial write Error"
    puts e
  end

  def proc_help()
    puts "Dumbass.... "
    puts CMD_DESC.map{|k,v| "#{k}: #{v}"}.join("\n")
  end

  def proc_read(address)
    msg = SerialDriveMsg.new(:cmd => 0x03, :address => address, :data_2 => 0x01)
    serconn.write(msg.to_msg)
    return wait_for_response
  end

  def proc_write(address, value, do_wait_for_response = false)
     msg = SerialDriveMsg.new(:cmd => 0x06, :address => address, data_2: value)
     serconn.write(msg.to_msg)

     return if do_wait_for_response
       wait_for_response
      else
        ""
      end

  end

  def proc_moveto(address, position)
    ##TODO
  end

  def wait_for_response
    Timeout.timeout(CONN_TIMEOUT) do 
      response = ""

      while response != ""
        response = serconn.read(32)
      end 

      return response
    end
  rescue Timeout::Error
    puts "Timed out waiting for serial response."
    return nil
  end

  def proc_exit()
    return nil
  end
end

class SerialDriveMsg
  def initialize(options = {})
    @address = 0x01
    @cmd = options[:cmd] || 0x03
    @data_address = options.fetch(:address)
    @data_1 = options.fetch(:data_1, 0x00)
    @data_2 = options.fetch(:data_2, 0x00)
  end

  def to_msg()
    str = ""
    str << sprintf("%02X", @address)
    str << sprintf("%02X", @cmd)
    str << sprintf("%04X", @data_address)
    str << sprintf("%02X", @data_1)
    str << sprintf("%02X", @data_2)
    str << sprintf("%02X", checksum)
    str << sprintf("%02X", 0x0D)
    str << sprintf("%02X", 0x0A)
    str
  end

  def checksum()
    (-(@address + @cmd + @data_address + @data_1 + @data_2) & 0xFF)
  end

end


#SerialInterfaceUtil.start(ARGV)