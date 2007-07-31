#!/usr/bin/ruby
$LOAD_PATH.unshift Dir.pwd.sub(%r{(bdb-\d\.\d\.\d/).*}, '\1src')

require 'bdb'
require 'getoptlong'

$master = false

def process_args
   parser = GetoptLong.new
   parser.set_options(
                      ['--master', '-M', GetoptLong::NO_ARGUMENT],
                      ['--client', '-C', GetoptLong::NO_ARGUMENT],
                      ['--verbose', '-v', GetoptLong::NO_ARGUMENT],
                      ['--home',   '-h', GetoptLong::REQUIRED_ARGUMENT],
                      ['--me',   '-m', GetoptLong::REQUIRED_ARGUMENT],
                      ['--nsites',  '-n', GetoptLong::REQUIRED_ARGUMENT],
                      ['--other',  '-o', GetoptLong::REQUIRED_ARGUMENT],
                      ['--priority', '-p', GetoptLong::REQUIRED_ARGUMENT]
                      )
   policy = BDB::REP_ELECTION
   home = "test"
   message = {
      BDB::EVENT_PANIC => "EVENT_PANIC",
      BDB::EVENT_REP_STARTUPDONE => "EVENT_REP_STARTUPDONE",
      BDB::EVENT_REP_CLIENT => "EVENT_REP_CLIENT",
      BDB::EVENT_REP_ELECTED => "EVENT_REP_ELECTED",
      BDB::EVENT_REP_MASTER => "EVENT_REP_MASTER",
      BDB::EVENT_REP_NEWMASTER => "EVENT_REP_NEWMASTER",
      BDB::EVENT_REP_PERM_FAILED => "EVENT_REP_PERM_FAILED",
      BDB::EVENT_WRITE_FAILED => "EVENT_WRITE_FAILED"
   }
   opt = {'set_event_notify' => lambda {|e|
         puts "Received #{message[e]}"
         $master = true if e == BDB::EVENT_REP_MASTER
         $master = false if e == BDB::EVENT_REP_CLIENT
      }}
   opt['rep_set_priority'] = 100
   parser.each do |name, arg|
      case name
      when '--master'
         policy = BDB::REP_MASTER
      when '--client'
         policy = BDB::REP_CLIENT
      when '--home'
         home = arg
      when '--me'
         me = arg
         raise "bad host specification, expected host:port" unless /:\d+$/ =~ arg
         me = me.split(/:/)
         raise "bad host specification, expected host:port" unless me.size == 2
         opt['repmgr_set_local_site'] = me[0], me[1].to_i
      when '--nsites'
         opt['rep_set_nsites'] = arg.to_i
      when '--other'
         me = arg
         raise "bad host specification, expected host:port" unless /:\d+$/ =~ arg
         me = me.split(/:/)
         raise "bad host specification, expected host:port" unless me.size == 2
         opt['repmgr_add_remote_site'] = me[0], me[1].to_i
      when '--priority'
         opt['rep_set_priorityrity'] = arg.to_i
      when '--verbose'
         opt['set_verb_replication'] = 1
      end
   end
   unless home && opt['repmgr_set_local_site']
      raise "home or local_site must be specified"
   end
   [home, policy, opt]
end

home, policy, opt = process_args

Dir.mkdir(home) unless File.exist? home

env = BDB::Env.new(home,
                   BDB::THREAD | BDB::CREATE | BDB::INIT_TRANSACTION | BDB::INIT_REP,
                   opt)
env.repmgr_start(3, policy)
flags = 0
flags = BDB::CREATE if $master
bdb = nil
loop do
   print "QUOTESERVER#{$master?'':' (read only)'}> "
   line = STDIN.gets
   break if !line
   line = line.strip.downcase
   case line
   when "exit", "quit"
      puts "at exit"
      break
   end
   key, value = line.split(/\s+/, 2)
   if bdb.nil?
      begin
         bdb = env.open_db(BDB::BTREE, 'quote.db', nil, BDB::AUTO_COMMIT | flags)
      rescue
         if $master
            puts $!
            exit
         end
         puts "Not yet available"
         next
      end
   end
   if key.nil?
      puts "\tSymbol\tPrice"
      puts "\t======\t====="
      bdb.each {|k, v| puts "\t#{k}\t#{v}" }
      next
   end
   if !$master
      puts "Can't update at client"
      next
   end
   bdb.put(key, value, BDB::AUTO_COMMIT)
end

bdb.close
env.close

