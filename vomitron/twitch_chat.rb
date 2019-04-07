require 'cinch'
require 'redis'
require "json"
require "ostruct"
require_relative "cycle_manager"

include CycleManager


$msg_send = []


def process_vote(vote_text, user)
  vote = vote_text[1].downcase
  return false unless current_vote_options.include?(vote)

  user = "twitch:" + user.nick

  set_user_vote(user, vote)

  return true
end

class TimedPlugin
  include Cinch::Plugin

  timer 1, method: :timed
  def timed
    if val = check_messages
      JSON.parse(val).each do |cmd|
        Channel(ENV.fetch("TWITCH_CHANNEL")).send(cmd)
      end
      $redis.del("message_queue")
    end
  end
end

bot = Cinch::Bot.new do 
  configure do |c|
    c.server = ENV.fetch("TWITCH_HOST")
    c.channels = [ENV.fetch("TWITCH_CHANNEL")]
    c.password = ENV.fetch("OAUTH_CHAT_KEY")
    c.nick = ENV.fetch("OAUTH_CHAT_NICK","vomitron_bot")
    c.user = ENV.fetch("OAUTH_CHAT_NICK","vomitron_bot")
    c.plugins.plugins = [TimedPlugin]
  end

  on :message, /^\>/ do |m|
    if !(process_vote(m.message, m.user))
      m.reply "Didn't get that vote."
    end
  end
end

bot.start
