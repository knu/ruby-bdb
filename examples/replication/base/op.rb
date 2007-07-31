#!/usr/bin/ruby
require 'getoptlong'
parser = GetoptLong.new
parser.set_options(
                  ['--master', '-M', GetoptLong::NO_ARGUMENT],
                  ['--client', '-C', GetoptLong::NO_ARGUMENT],
                  ['--home',   '-h', GetoptLong::REQUIRED_ARGUMENT],
                  ['--me',   '-m', GetoptLong::REQUIRED_ARGUMENT],
                  ['--nsites',  '-n', GetoptLong::REQUIRED_ARGUMENT],
                  ['--other',  '-o', GetoptLong::REQUIRED_ARGUMENT],
                  ['--priority', '-p', GetoptLong::REQUIRED_ARGUMENT]
                  )
$home, $port, $others = "test", 5000, []
$priority, $totalsites, $timeout = 10, 4, 1000000
client = false
parser.each do |name, arg|
   case name
   when '--master'
      raise "can't be a client and a master" if client
      $master = true
   when '--client'
      raise "can't be a client and a master" if $master
      client = true
   when '--home'
      $home = arg
   when '--me'
      $me = arg
      $port = $me.split(/:/)[1]
   when '--nsites'
      $totalsites = arg.to_i
   when '--other'
      $others ||= []
      raise "bad host specification, expected host:port" unless /:\d+$/ =~ arg
      $others.push arg.split(/:/)
   when '--priority'
      $priority = arg.to_i
   end
end
