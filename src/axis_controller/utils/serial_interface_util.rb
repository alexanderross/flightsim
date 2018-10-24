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
      self.new(port: args.first || '/dev/tty.usbserial')
    end
  end


  def initialize(port:, baud: 38400, data_bits: 8, parity: :even)

    @serconn = if ENV["TEST"]
      TestSerConn.new()
    else
      Serial.new(port, baud, data_bits, parity)
    end

    run
  end

  def run
    while get_input
      #party!!
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
    return true
  end

  def proc_read(address)
    msg = SerialDriveMsg.new(:cmd => 0x03, :address => address.to_i, :data => 0x01)
    serconn.write(msg.to_msg)
    puts wait_for_response

    return true
  end

  def proc_write(address, value, do_wait_for_response = false)
    msg = SerialDriveMsg.new(:cmd => 0x06, :address => address.to_i, data: value.to_i)
    serconn.write(msg.to_msg)

    if do_wait_for_response
      puts wait_for_response
    end

    return true

  end

  def proc_moveto(address, position)
    ##TODO
  end

  def wait_for_response
    Timeout.timeout(CONN_TIMEOUT) do 
      response = ""

      while response == ""
        response = serconn.read(32)
      end 

      return response
    end
  rescue Timeout::Error
    puts "Timed out waiting for serial response."
    return nil
  end

  def proc_exit(exit="yeah")
    puts "BYE!"
    exit
    return false
  end
end

class SerialDriveMsg
  def initialize(options = {})
    @address = 0x01
    @cmd = options[:cmd] || 0x03
    data_address = options.fetch(:address)
    @data_address_hi, @data_address_lo = split_4b_hex(data_address)

    @data_1, @data_2 = split_4b_hex(options.fetch(:data, 0x0001))

  end

  def to_hx(item)
    sprintf("%02X", item)#.split('').map{|i| "%02X" % i.ord}.join("")
  end

  def to_msg()
    str = ':'
    str << to_hx(@address)
    str << to_hx(@cmd)
    str << to_hx(@data_address_hi)
    str << to_hx(@data_address_lo)
    str << to_hx(@data_1)
    str << to_hx(@data_2)
    str << to_hx(checksum)
    str << "\r\n"
    str
  end

  def checksum()
    (-(@address + @cmd + @data_address_lo + @data_1 + @data_2) & 0xFF)
  end

  private

  #For a given hex represented in 4 bytes (0x0000), split them into halves and return the associated integers.
  def split_4b_hex(num)
    return ("%04X" % num).to_s.split('').each_slice(2).to_a.map{|i| i.join.to_i(16)}
  end

end

class TestSerConn

  def read(bytes)
    return ":)"
  end

  def write(msg)
    puts "TEST SER WRITES"
    puts msg
  end

end

#WRITE 4040 to 178 - :010600B20FC870
#      2020          :010600B207E45C


SerialInterfaceUtil.start(ARGV)