require 'mkmf'
load './myconfig'

def addld(path, lib)
   libs = if lib.kind_of?(Array)
	     "-l" + lib.join(" -l")
	  else
	     "-l#{lib}"
	  end
   if path
      case Config::CONFIG["arch"]
      when /solaris2/
	 $LDFLAGS += " -L#{path} -R#{path} #{libs}"
      when /linux/
	 $LDFLAGS += " -Wl,-rpath,#{path} -L#{path} #{libs}"
      else
	 $LDFLAGS += " -L#{path} #{libs}"
      end
   else
      $LDFLAGS += " #{libs}"
   end
end

$CFLAGS += " -I. -I../src"

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

$order.each do |key|
   val = $library[key]
   if val.kind_of?(Array) && val.size == 2
      addld(*val)
   end
end

if CONFIG["LDSHARED"] == "gcc -shared"
   CONFIG["LDSHARED"] = "g++ -shared"
end

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
\t@-(cd docs; #{CONFIG['RUBY_INSTALL_NAME']} b.rb bdbxml; rdoc bdbxml.rb)

rd2: html

html: $(HTML)

test: $(DLLIB)
   EOF
   Dir.foreach('tests') do |x|
      next if /^\./ =~ x || /(_\.rb|~)$/ =~ x
      next if FileTest.directory?(x)
      make.print "\t#{CONFIG['RUBY_INSTALL_NAME']} tests/#{x}\n"
   end
ensure
   make.close
end



