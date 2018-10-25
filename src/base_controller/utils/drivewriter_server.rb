require 'rack/app'
require 'inline'
require 'fiddle'

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
    begin
      ShmemWriter.write(axis, register, axis)
      return "Wrote #{value} to #{register} on #{axis}"
    rescue StandardError => e
      return e.message
    end
  end

end

class ShmemWriter
  inline do |builder|
    builder.include '<sys/types.h>'
    builder.include '<sys/ipc.h>'
    builder.include '<sys/shm.h>'
    builder.c 'int getShmid(int key) {
      return shmget(key, 256, 0644 | IPC_CREAT);
    }'
    builder.c 'int getMem(int id) {
      return shmat(id, NULL, 0);
    }'
    builder.c 'int removeMem(long id) {
      return shmdt(id);
    }'
  end

  def self.write(axis, register, value)
    basic_id = self.new.getShmid(1337)
    address = self.new.getMem(basic_id)
    pointer = Fiddle::Pointer.new(address, 256)

    string = axis.upcase + sprintf("%03d", register.to_i) + sprintf("%06d", value.to_i)
    second_pointer = Fiddle::Pointer.new(string.object_id)
    pointer = second_pointer
  end
end