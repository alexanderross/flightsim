require 'rack/app'

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
  	#TODO - get this into shared mem to be picked up by the rfaxiscomm program
  	puts "WRITING #{value} to #{register} on #{axis}"
  end

end