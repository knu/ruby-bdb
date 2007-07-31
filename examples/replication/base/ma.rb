#!/usr/bin/ruby
def do_master
   $master = BDB::MC::ENVID
   $env.rep_start nil, BDB::REP_MASTER
   db = $env.open_db(BDB::BTREE, $db,  nil, "a")
   Thread.new do
      while true
	 print "#{Time.now}> "
	 $stdout.flush
	 line = gets
	 break if !line
	 line = line.chomp.downcase
	 if /(.*?)\s/ =~ line
	    db[$1] = $'
	 else
	    case line
	    when "exit", "quit"
	       puts "at exit"
	       db.each do |k, v|
		  puts "#{k} -- #{v}"
	       end
	       exit
	    else
	       puts "error : TICKER VALUE"
	    end
	 end
      end
   end
end
