#!/usr/bin/ruby
def do_client
   $env.rep_start $me, BDB::REP_CLIENT
   sleep 5
   Thread.new do
      db = ''
      begin
         db = $env.open_db(BDB::BTREE, $db,  nil, "r")
      rescue
         sleep 5
         retry
      end
      while true
	 break if $master == BDB::MC::ENVID
	 display_message("content #{Time.now}")
	 db.each do |k, v|
	    puts "#{k} -- #{v}"
	 end
	 sleep 3
      end
      db.close
   end

   Thread.new do
      count = 0
      while $master == BDB::EID_INVALID
	 if (count % 2) == 0
	       $env.rep_start $me, BDB::REP_CLIENT
	 else
	    begin
	       $master = $env.rep_elect *$env.param
	    rescue BDB::RepUnavail
	       if ($env.empty? and $env.priority > 0) || count > 10
		  do_master
	       end
	    end
	 end
	 count += 1
         sleep 1
      end
   end

end
