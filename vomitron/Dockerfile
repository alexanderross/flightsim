FROM ruby:2.4.0

RUN apt-get update
RUN apt-get install libsasl2-dev


# Set an environment variable where the Rails app is installed to inside of Docker image
ENV RAILS_ROOT /var/www/rover
ENV RACK_ENV production
ENV MIN_THREADS 10
ENV MAX_THREADS 64
RUN mkdir -p $RAILS_ROOT 

# Set working directory
WORKDIR $RAILS_ROOT

# Setting env up - static as this is not exactly a production thing.. Well sort of.. Like a homework production thing? Idk I havent had homework in a while.

# Adding gems
RUN gem install slim
RUN gem install redis
RUN gem install tilt
RUN gem install memcached
RUN gem install cinch
RUN gem install rack-app
RUN gem install puma
RUN gem install twilio-ruby
# Adding project files
COPY . .

EXPOSE 3000

CMD ["rackup", "-o", "0.0.0.0", "-p", "3000"]