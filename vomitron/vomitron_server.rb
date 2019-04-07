require 'rack/app'
require 'slim'
require 'tilt'
require 'timeout' #So what, I used timeout :D
require 'yaml'
require 'json'
require 'memcached'
require 'redis'
require 'net/http'
require 'uri'
require 'twilio-ruby'
require_relative 'cycle_manager'

$cache = Memcached.new(ENV.fetch("MEMCACHED_URL", "localhost:11211"))

UPDATE_INTERVAL = 3


class App < Rack::App

  include CycleManager

  def set_cache_headers(expire=3600)
    response.headers['cache-control'] = "max-age=#{expire}"
  end

  get '/' do
    Tilt.new("views/index.html.slim").render self
  end

  get '/data.json' do
    vote_data.to_json
  end

  get '/txtcallback' do
    user = params['From'].downcase
    vote = params['Body'].strip.downcase[0]
    success = set_user_vote("sms#{user}",vote)

    response.headers['Content-Type']= 'application/xml; charset=utf-8'

    "<Response></Response>"
  end


  get '/suspend' do
    return "NOT ALLOWED" unless ENV.fetch("ALLOWED_IP", "127.0.0.1") == request.ip || ENV.fetch("ALLOW_ALL", true) == true
    $cache.delete("cycle_data")
    stop_machine if(params["nostop"].nil?)
    pause_cycle
  end

  get '/commence' do
    return "NOT ALLOWED" unless ENV.fetch("ALLOWED_IP", "127.0.0.1") == request.ip || ENV.fetch("ALLOW_ALL", true) == true
    $cache.delete("cycle_data")
    continue_cycle
  end

  get '/simvotes' do
    return "NOT ALLOWED" unless ENV.fetch("ALLOWED_IP", "127.0.0.1") == request.ip || ENV.fetch("ALLOW_ALL", true) == true
    count = params["ct"].to_i || 5000
    opts=['a','b','c','d','e','f']
    count.times do
      set_user_vote("test#{rand(900000)}", opts.sample)
    end
  end

  get '/autopick' do
    if(params["srs"])
      set_autopick(params["state"])
    end
  end

  def vote_data
    data = $cache.get(["cycle_data"])
    if(data == {})
      #READ FROM REDIS
      data = current_cycle_data
      $cache.set("cycle_data", data.to_json, 5)
    else
      data = JSON.parse(data["cycle_data"])
    end
    data["sec_remaining"] = data["cycle_end"].to_i - Time.now.to_i
    to = curr_top_option
    data["top_char"] = to.nil? ? nil : to[0]
    data["fs_stats"] = check_fs_state
    data
  end

  def check_fs_state
    data = $cache.get(["fs_state"])
    if(data == {})
      indata = {}
      uri = URI.parse(FS_HOST+"/status")
      begin
        #raise Exception.new()
        response =  Net::HTTP.get_response uri
        indata = JSON.parse(response.body).merge({:server => "UP"})
      rescue Exception => e
        puts e.message
        indata[:server] = "DOWN"
      end
       $cache.set("fs_state", indata.to_json, 10)
       return indata
    else
      return JSON.parse(data["fs_state"])
    end
  end

end