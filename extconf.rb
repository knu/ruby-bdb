#!/usr/bin/ruby
require 'mkmf'

def rule(target, clean = nil)
   wr = "#{target}:
\t@for subdir in $(SUBDIRS); do \\
\t\t(cd $${subdir} && $(MAKE) #{target}); \\
\tdone;
"
   if clean != nil
      wr << "\t@-rm tmp/* tests/tmp/* 2> /dev/null\n"
      wr << "\t@rm Makefile\n" if clean
   end
   wr
end

subdirs = Dir["*"].select do |subdir|
   File.file?(subdir + "/extconf.rb")
end

begin
   make = open("Makefile", "w")
   make.print <<-EOF
SUBDIRS = #{subdirs.join(' ')}

#{rule('all')}
#{rule('clean', false)}
#{rule('distclean', true)}
#{rule('realclean', true)}
#{rule('install')}
#{rule('site-install')}
#{rule('unknown')}
%.html: %.rd
	rd2 $< > ${<:%.rd=%.html}

   EOF
   make.print "HTML = bdb.html"
   Dir.foreach('docs') do |x|
      make.print " \\\n\tdocs/#{x}" if x.sub!(/\.rd$/, ".html")
   end
   make.puts
   make.print <<-EOF

html: $(HTML)

test: $(DLLIB)
   EOF
   Dir.foreach('tests') do |x|
      next if /^\./ =~ x || /(_\.rb|~)$/ =~ x
      next if FileTest.directory?(x)
      make.print "\truby tests/#{x}\n"
   end
ensure
   make.close
end

subdirs.each do |subdir|
   STDERR.puts("#{$0}: Entering directory `#{subdir}'")
   Dir.chdir(subdir)
   system("#{Config::CONFIG['RUBY_INSTALL_NAME']} extconf.rb " + ARGV.join(" "))
   Dir.chdir("..")
   STDERR.puts("#{$0}: Leaving directory `#{subdir}'")
end
