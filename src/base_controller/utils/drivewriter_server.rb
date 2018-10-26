require 'rack/app'

RFCOMM_CMD_PATH = "/tmp/rfcmdpath"


class App < Rack::App

  desc 'Write a value to a register for a drive'
  validate_params do
    required 'axis', :class => String, :desc => "Axis to write to ('roll' or 'pitch')", :example => 'roll'
    required 'r', :class => String, :desc => 'The register to write to', :example => '177'
    required 'v', :class => String, :desc => 'The value to write', :example => '1200'
  end
  post '/write' do
    axis = params['axis'].downcase[0]
    write_to_drive(axis, params['r'], params['v'])
  end



  def write_to_drive(axis, register, value)
    #Fine we'll use a damn file
    begin
      File.open(yourfile, 'w') { |file| file.write(axis.upcase + sprintf("%03d", register.to_i) + sprintf("%06d", value.to_i)) }
      return "Wrote #{value} to #{register} on #{axis}"
    rescue StandardError => e
      return e.message
    end
  end

end