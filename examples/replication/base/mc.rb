#!/usr/bin/ruby
$LOAD_PATH.unshift Dir.pwd.sub(%r{(bdb-\d\.\d\.\d/).*}, '\1src')

require 'bdb'
require 'mutex_m'
require 'socket'

module BDB
   class MC < Env
      extend Mutex_m

      ENVID = 1

      attr_accessor :sites, :priority, :timeout

      def initialize(home = nil, sites = 4, pri = 10, timeout = 2000000)
	 @sites, @priority, @timeout = sites, pri, timeout
         @mach = {}
         super(home, BDB::THREAD | BDB::CREATE | BDB::INIT_TRANSACTION | BDB::INIT_REP,
               "set_verb_replication" => 0)
      end

      def param
	 [(@mach.empty? ? @sites : @mach.size), 1]
      end

      def empty?
	 @mach.empty?
      end

      def mach_name(eid)
	 @mach[eid] && @mach[eid][0, 2].join(':')
      end

      def add_mach(f, h, a)
	 @eid ||= 2
	 self.class.synchronize do
	    @mach.each do |eid, inf|
	       host, addr, = inf
	       return eid if h == host && a == addr
	    end
	    @eid += 1
	    @mach[@eid] = [h, a, f]
	 end
	 @eid
      end

      def remove_mach(eid)
	 self.class.synchronize do
	    @mach.delete(eid)
	 end
      end

      def comm(eid)
	 return nil unless @mach[eid]
         res, s = "", @mach[eid][2]
	 nb = s.sysread(10)
	 nb = nb.to_i
	 while res.size < nb
	    res << s.sysread(nb - res.size)
	 end
	 return nil if res.size < nb
	 Marshal.load res
      rescue
	 remove_mach(eid)
	 return nil
      end

      def bdb_rep_transport(control, rec, lsn, eid, flags)
	 self.class.synchronize do
	    case eid
	    when BDB::EID_BROADCAST
	       new = {}
	       aa = Marshal.dump([rec, control])
	       @mach.each do |neid, mach|
		  begin
		     mach[2].syswrite("%010d" % aa.size)
		     if mach[2].syswrite(aa) == aa.size
			new[neid] = mach
		     end
		  rescue
		  end
	       end
	       @mach = new
	       return 0
	    when BDB::EID_INVALID
	       puts "received INVALID"
	       return 0
	    else
	       @mach.each do |neid, mach|
		  if neid == eid
		     aa = Marshal.dump([rec, control])
		     begin
			mach[2].syswrite("%010d" % aa.size)
			if mach[2].syswrite(aa) == aa.size
			   return 0
			end
		     rescue
		     end
		     return BDB::REP_UNAVAIL
		  end
	       end
	       raise BDB::Fatal, "cannot find #{eid}"
	    end
	 end
      end

      def open_db(*args)
         db = super
         db.instance_eval <<-EOT
         def []=(a, b)
	    env.begin(BDB::TXN_COMMIT, self) do |txn, db1|
	       db1.put(a, b)
	    end
         end
         EOT
	 db
      end
   end
end
