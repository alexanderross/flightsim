require_relative "cycle_manager"
require 'yaml'
require 'net/http'

CONTINUE_VOTELESS = false
AUTOPICK_TIME = 10

class RunnableItem
  include CycleManager

  attr_reader :probability, :id, :label, :commands, :duration
  def initialize(instructions)
    @instructions = Array(instructions)
    build_attrs
  end

  def build_attrs
    if(@instructions.size == 1)
      @probability = @instructions[0].probability
      @duration = @instructions[0].duration_s
      @id = @instructions[0].id
      @label = @instructions[0].label
      @commands = [@instructions[0].command]
    else
      build_composite_args
    end
  end

  #Executes command and returns how long it should be run for.
  def execute
    execute_cmd(execution_path)
    if self.duration
      return Time.now.to_i + self.duration
    else
      return 0
    end
  end

  def build_composite_args
    @probability = @instructions.sum(&:probability) / @instructions.size
    @duration = @instructions.reject{|i| i.manual}.map{|itm| itm.duration_s.to_i).tap{|ds| ds.reduce(:+) / ds.size.to_f}.round(0)
    @id = @instructions.map(&:id).join("_")
    @label = @instructions.map(&:label).join(" and ")
    @commands = @instructions.map(&:command)

  end

  def execution_path
    auto_instructions = @instructions.reject{|i| i.manual}

    if auto_instructions.length == 2 

      writes = []
      auto_instructions.each do |ai|
        inst_prefix = ai.axis[0]
        speed = ai.command.split("#{inst_prefix}c=176|")[1].split("&")[0]
        if(rand(2) == 1)
          speed = "-#{speed}"
        end
        writes << "#{inst_prefix}c=176|#{speed}"
      end

      return "?"+writes.join("&")

    elsif auto_instructions.length == 1

      return auto_instructions.first.command

    end
  end
end

class RunnerOptionFactory
  attr_reader :item_dict
  def initialize(items)
    @items = items.map{ |i| OpenStruct.new(i)}
    @probabilistic_pool = []
    @item_dict = {}
    create_item_pool
  end

  def create_item_pool
    pool = []
    pairable_roll_cmds = @items.select{|i| i.pairable == true && i.axis == "roll"}
    pairable_pitch_cmds = @items.select{|i| i.pairable == true && i.axis == "pitch"}

    pairable_pitch_cmds.each do |c|
      pairable_roll_cmds.each do |cc|
        pool << RunnableItem.new([c,cc])
      end
    end

    @items.each do |i|
      pool << RunnableItem.new(i)
    end

    pool.each do |item|
      @item_dict[item.id] = item
      (item.probability * 10).to_i.times do
        @probabilistic_pool << item
      end
    end

    puts "Probability pool contains #{@probabilistic_pool.size} items."
  end

  def grab_item
    @probabilistic_pool.sample
  end

  def get_new_round(num_items = 6)
    round_items = []
    raise Exception.new("not enough items to do that.") unless @item_dict.keys.size >= num_items

    while round_items.length < num_items
      new_item = grab_item
      round_items << new_item unless round_items.include?(new_item)
    end

    return round_items
  end
end

class Runner
  include CycleManager

  def initialize
    @config = YAML.load_file(Dir.pwd+"/config.yml")
    @opt_factory = RunnerOptionFactory.new(@config["items"])

    @cycle_interval = ENV.fetch("CYCLE_INTERVAL", @config["cycle_interval"]).to_i

    if cycle_end_seconds.to_i > Time.now.to_i
      puts "CONTINUING"
      @run_start = Time.now.to_i
      @run_end = cycle_end_seconds.to_i
    end
  end

  def extend_cycle
    puts "EXTENDING CYCLE"
    @run_start = Time.now.to_i
    @run_end = @run_start + @config["cycle_interval"].to_i
    set_cycle_end(@run_end)
  end

  def reset_cycle
    puts "RESETTING CYCLE"
    puts "CYCLING DISABLED" unless cycle_enabled?
    
    new_items = @opt_factory.get_new_round
    initiate_cycle(new_items)

    @run_start = Time.now.to_i
    @run_end = @run_start + @cycle_interval
    set_cycle_end(@run_end)
    post_message("Starting a new round. Ends in #{@cycle_interval} seconds")
  end

  def finish_cycle
    data = current_cycle_data

    total = data["total_votes"].to_i
    stop_machine

    if(total == 0)
      if(CONTINUE_VOTELESS)
        post_message("There were no votes cast, extending the voting cycle. Ends in #{@cycle_interval} seconds")
        extend_cycle
      else
        reset_cycle
      end
    else
      top_option = [nil, 0]

      data["votes"].each do |k,v|
        top_option = [k,v.to_i] if v.to_i > top_option[1]
      end

      if(top_option[0].nil?)
        post_message("There was no winner. Bummer.")
      else
        winner = OpenStruct.new(data["options"][top_option[0]])

        post_message("With #{commas(top_option[1].to_i)} of #{commas(total)} votes, #{winner.label} is the winner!")
        set_last_motion(winner)
        puts winner
        cmd_end_time = @opt_factory.item_dict[winner.id].execute
        @cmd_die_time = cmd_end_time if((cmd_end_time) != 0)
      end
      reset_cycle
    end
  end

  def kill_current_cmd_if_needs_to_die()
    if(@cmd_die_time != 0)
      if(Time.now >= @cmd_die_time)
        stop_machine
        @cmd_die_time = 0
      end
    end
  end

  def commas(val)
    val.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse
  end

  def run
    puts "STARTING RUNNER"
    while true
      sleep 1
      reset_cycle if(@run_start == nil)

      kill_current_cmd_if_needs_to_die()

      if Time.now.to_i > @run_end
        finish_cycle
      elsif !cycle_enabled?
        @run_end = @run_end + 1
      elsif autopick_enabled? && ((@run_end - Time.now.to_i) == AUTOPICK_TIME)
        vote =(97 + rand(5)).chr
        puts("AUTOVOTE #{vote}")
        set_user_vote("AUTOVOTE", vote)
      end

      set_cycle_end(@run_end)

    end
  end

end

Runner.new.run