require 'json'
require 'redis'

$redis ||= Redis.new(url: ENV.fetch("REDIS_URL", "redis://127.0.0.1:6379/0"))
FS_HOST = "http://#{ENV.fetch('FS_HOST')}:#{ENV.fetch('FS_PORT')}"

module CycleManager

  def current_cycle
    $redis.get("chat_cycle_id")
  end

  def increment_cycle
    $redis.incr("chat_cycle_id")
  end

  def set_user_vote(user, vote)
    if($redis.exists("user_vote_#{current_cycle}_#{user}"))
      old_vote = $redis.get("user_vote_#{current_cycle}_#{user}")
      $redis.decr("cycle_vote_#{current_cycle}_#{old_vote}")
    else
      $redis.incr("cycle_total_votes_#{current_cycle}")
    end

    $redis.incr("cycle_vote_#{current_cycle}_#{vote}")
    $redis.set("user_vote_#{current_cycle}_#{user}", vote)
  end

  def cycle_data
    if $redis.exists("cycle_data_#{current_cycle}")
      JSON.parse($redis.get("cycle_data_#{current_cycle}"))
    else
      return nil
    end
  end

  def set_autopick(state)
    $redis.set("auto_pick", state)
  end

  def autopick_enabled?
    $redis.get("auto_pick") == "1"
  end

  def stop_machine
    execute_cmd("?rc=176|0&pc=176|0")
  end

  def execute_cmd(cmd)
    begin
      base_path = [FS_HOST, cmd].join("/write_dual")
      uri = URI(base_path)
      Net::HTTP.get(uri)
    rescue Exception => e
      puts e.message
    end
  end

  def curr_top_option
    return nil unless total_votes.to_i > 0

    data = current_cycle_data

    top_option = [nil, 0]

    data["votes"].each do |k,v|
      top_option = [k,v.to_i] if v.to_i > top_option[1]
    end

    return top_option
  end

  def set_last_motion(last_motion_in)
    $redis.set("last_cycle_motion", last_motion_in.to_h.to_json)
    motions = []
    if $redis.get("cycle_motions")
      motions = JSON.parse($redis.get("cycle_motions"))
    end
    motions << last_motion_in.id
    $redis.set("cycle_motions", motions.to_json)
  end

  def last_motion
    JSON.parse($redis.get("last_cycle_motion")) if($redis.exists("last_cycle_motion"))
  end

  def vote_options
    options = $redis.get("cycle_valid_votes_#{current_cycle}")
    return {} unless options
    JSON.parse($redis.get("cycle_valid_votes_#{current_cycle}"))
  end

  def total_votes
    $redis.get("cycle_total_votes_#{current_cycle}")
  end

  def get_current_vote_num_for_option(option)
    $redis.get("cycle_vote_#{current_cycle}_#{option}")
  end

  def current_vote_options
    vote_options
  end

  def pause_cycle
    $redis.set("cycle_enabled",0)
    post_message("Voting has been stopped.")
  end

  def cycle_enabled?
    $redis.get("cycle_enabled") == "1"
  end

  def continue_cycle
    $redis.set("cycle_enabled",1)
    post_message("The voting round has started and will end in #{cycle_end_seconds.to_i - Time.now.to_i} seconds.")
  end

  def set_cycle_end(end_in_seconds)
    $redis.set("cycle_end_seconds", end_in_seconds)
  end

  def cycle_end_seconds
    $redis.get("cycle_end_seconds")
  end

  def current_cycle_data
    data = {}
    data["total_votes"] = total_votes
    data["current_cycle"] = current_cycle
    data["cycle_end"] = cycle_end_seconds
    data["cycle_enabled"] = $redis.get("cycle_enabled") || '0'
    data["cycle_enabled_s"] = cycle_enabled?
    data["last_motion"] = last_motion
    data["autopick"] = $redis.get("auto_pick") || '0'

    data["votes"] = {}
    current_vote_options.each do |v|
      data["votes"][v] = get_current_vote_num_for_option(v)
    end

    cdata = cycle_data
    if(cdata)
      data["options"] = cdata["options"]
      data["options"].keys.each do |k|
        data["options"][k] = {"id" => data["options"][k], "label" => cdata["labels"][k]}
      end
    end

    data

  end

  def initiate_cycle(options)
    cycle = $redis.incr("chat_cycle_id")
    vote_data = {"options" => {}, "labels" => {}}

    options.each_with_index do |option,i|
      vote_char = (97 + i).chr
      vote_data["options"][vote_char] = option.id
      vote_data["labels"][vote_char] = option.label
      $redis.set("cycle_vote_#{cycle}_#{vote_char}", 0)
    end

    $redis.set("cycle_data_#{cycle}", vote_data.to_json)

    $redis.set("cycle_valid_votes_#{cycle}", vote_data["options"].keys.to_json)

    $redis.set("cycle_total_votes_#{cycle}", 0)
  end

  def check_messages
    $redis.get("message_queue")
  end

  def post_message(message)
    if val = check_messages
      val = JSON.parse(val)
      val << message
      $redis.set("message_queue", val.to_json)

    else
      $redis.set("message_queue",[message].to_json)
    end
  end

end