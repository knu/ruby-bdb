# VERIFY 'myconfig' then comment this line

require 'mkmf'
load './myconfig'

$DLDFLAGS ||= ""
$LDFLAGS ||= ""

def addld(key, path, lib)
   libs = if lib.kind_of?(Array)
	     "-l" + lib.join(" -l")
	  else
	     "-l#{lib}"
	  end
   if path
      case Config::CONFIG["arch"]
      when /solaris2/
	 libs = " -L#{path} -R#{path} #{libs}"
      when /linux/
	 libs = " -Wl,-rpath,#{path} -L#{path} #{libs}"
      else
	 libs = " -L#{path} #{libs}"
      end
   end
   $stderr.puts "\t#{key}\tusing ... #{libs}"
   $DLDFLAGS += " #{libs}"
   $LDFLAGS += " #{libs}"
end

$CFLAGS += " -I. -I../src"

$stderr.puts "INCLUDE"
$order.each do |key|
   val = $include[key]
   unless val.nil?
      res = if val.kind_of?(Array)
	       " -I" + val.join(" -I")
	    else
	       " -I#{val}"
	    end
      $stderr.puts "\t#{key}\tusing ... #{res}"
      $CFLAGS += res

   end
end

$CFLAGS += " -DBDB_NO_THREAD" if enable_config("thread") == false

$stderr.puts "\nLIBRARY"
$order.each do |key|
   val = $library[key]
   if val.kind_of?(Array) && val.size == 2
      addld(key, *val)
   end
end

if CONFIG["LDSHARED"] == "gcc -shared"
   CONFIG["LDSHARED"] = "g++ -shared"
end

if with_config("bdb-objs")
   bdb_obj = Dir["../src/*.#{$OBJEXT}"]
   if bdb_obj.size == 0
      puts <<-EOT

 ****************************************************
 Build bdb first, if you want to link bdbxml with bdb
 ****************************************************

      EOT
      exit
   end
   $objs = ["bdbxml.o"] + bdb_obj
   $CFLAGS += " -DBDB_LINK_OBJ"
end

have_func("rb_block_call")

require 'features.rb'
create_makefile('bdbxml')

begin
   make = open("Makefile", "a")
   make.print <<-EOF

%.html: %.rd
\trd2 $< > ${<:%.rd=%.html}

   EOF
   make.print "HTML = bdbxml.html"
   docs = Dir['docs/*.rd']
   docs.each {|x| make.print " \\\n\t#{x.sub(/\.rd$/, '.html')}" }
   make.print "\n\nRDOC = bdbxml.rd"
   docs.each {|x| make.print " \\\n\t#{x}" }
   make.puts
   make.print <<-EOF

rdoc: docs/doc/index.html

docs/doc/index.html: $(RDOC)
\t@-(cd docs; rdoc .)

ri:
\t@-(rdoc -r docs/*rb)

ri-site:
\t@-(rdoc -R docs/*rb)

rd2: html

html: $(HTML)

test: $(DLLIB)
   EOF
   Dir.glob('tests/*.rb') do |x|
      next if /(_\.rb|~)$/ =~ x
      next if FileTest.directory?(x)
      make.print "\t#{CONFIG['RUBY_INSTALL_NAME']} #{x}\n"
   end
ensure
   make.close
end



