#!/usr/bin/ruby
require './op.rb'
require './mc.rb'
require './ma.rb'
require './cl.rb'

Dir.mkdir($home) if !File.exist? $home

$db = "test"
$master = $master ? BDB::MC::ENVID : BDB::EID_INVALID

$env = BDB::MC.new($home, $totalsites, $priority, $timeout)

Thread.abort_on_exception = true

def display_message(mess)
   puts "===> #{mess}"
end

def elections(tiout = 2, max = 5)
   display_message("Beginning an election")
   count = 0
   begin
      $master = $env.rep_elect *$env.param
   rescue
      if count >= max
	 display_message("Forcing the election")
	 $master = BDB::MC::ENVID
      else
	 display_message("retrying ...")
	 count += 1
	 sleep tiout
	 retry
      end
   end
   if $master == BDB::MC::ENVID
      display_message("Won the election")
   else
      display_message("The new master is #{$env.mach_name($master)}")
   end
end

def hm_loop(sock, host, addr)
   eid = $env.add_mach(sock, host, addr)
   display_message("new client #$me -- #{host}:#{addr}")
   Thread.new do
      election = nil
      while true
         record, control = $env.comm(eid)
	 if ! record
	    break if $master == BDB::MC::ENVID || $master != eid
	    $master = BDB::EID_INVALID
	    elections(1, 2)
	    if $master == BDB::MC::ENVID
	       do_master
	    end
	    break
	 end
	 res, cdata, envid = $env.rep_process_message(control, record, eid)
	 case res
	 when BDB::REP_NEWSITE
	    display_message("REP_NEWSITE #{cdata}")
	    if cdata != $me
	       begin
		  connect_site(cdata.split(/:/))
	       rescue
		  break
	       end
	    end
	 when BDB::REP_HOLDELECTION
	    display_message("REP_HOLDELECTION")
	    if $master != BDB::MC::ENVID
	       if election
		  election.join
		  election = nil
	       end
	       election = Thread.new do
		  elections
		  if $master == BDB::MC::ENVID
		     $env.rep_start nil, BDB::REP_MASTER
		  end
		  election = nil
	       end
	    end
	 when BDB::REP_NEWMASTER
	    display_message("REP_NEWMASTER #{envid}")
	    $master = envid
	    do_master() if envid == BDB::MC::ENVID
	 when 0
	 when BDB::REP_DUPMASTER
	    display_message("REP_DUPMASTER")
	    $env.rep_start $me, BDB::REP_CLIENT
	    elections
	    if $master == BDB::MC::ENVID
	       do_master()
	    else
	       do_client()
	    end
         when BDB::REP_ISPERM
            display_message("REP_ISPERM")
         when BDB::REP_NOTPERM
            display_message("REP_NOTPERM")
         when BDB::REP_IGNORE
            display_message("REP_IGNORE")
	 else
            display_message("error #{res} #{cdata} #{envid}")
	 end
      end
      $env.remove_mach(eid)
      election.join if election
	 
   end
end

def connect_site(site)
   return if $me == site.join(":")
   s = TCPSocket.open(site[0], site[1])
   hm_loop(s, site[0], site[1])
end

connect_thread = Thread.new do
   server = TCPserver.open($port)
   loop do
      ns = server.accept
      addr = ns.peeraddr
      hm_loop(ns, addr[2], addr[1])
   end
end

connect_all = Thread.new do
   others = $others.dup
   while ! others.empty?
      todo = []
      others.each do |site|
         begin
            connect_site(site)
         rescue
            todo << site
         end
      end
      others = todo
      sleep 1
   end
end

if $master == BDB::MC::ENVID
   do_master
else
   do_client
end

connect_all.join
connect_thread.join
