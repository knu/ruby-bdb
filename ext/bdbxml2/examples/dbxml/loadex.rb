#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'
require 'getoptlong'
opt = GetoptLong.new(
		     ['--container', '-c', GetoptLong::REQUIRED_ARGUMENT],
		     ['--path', '-p', GetoptLong::REQUIRED_ARGUMENT],
		     ['--files', '-f', GetoptLong::REQUIRED_ARGUMENT],
		     ['--home', '-h', GetoptLong::REQUIRED_ARGUMENT],
                     ['--dir', '-d', GetoptLong::REQUIRED_ARGUMENT]
		     )
options = {'path' => ''}
opt.each {|name, value| options[name.sub(/^--/, '')] = value}
$files = ARGV.dup
$files ||= []
options['path'] << '/' if options['path'].size > 0

if options['dir']
   Dir["#{options['dir']}/*.xml"].each {|f| $files << f }
end

if options['files']
   IO.foreach(options['files']) {|line| $files << options['path'] + line.chomp}
end

if !$files || !options['container'] || !options['home'] 
   puts <<-EOT

This program loads XML data into an identified container and environment.
Provide the directory where you want to place your database environment, 
the name of the container to use, and the xml files you want inserted into
the container.

  -h <dbenv directory> -c <container> -f <filelist> -p <filepath> file1.xml ...
   EOT
   exit(-1)
end

env = BDB::Env.new(options['home'],BDB::CREATE|BDB::INIT_TRANSACTION)
man = env.manager
upd = man.create_update_context
man.begin do |txn|
   con = txn.open_container(options['container'], BDB::CREATE)
   $files.each do |f|
      doc = man.create_document
      doc.content = IO.readlines(f, nil)[0]
      /[^\/]*$/ =~ f
      doc.name = $&
      doc.set("http://dbxmlExamples/timestamp",  "timeStamp",  Time.now.to_s)
      con.put(doc, upd, BDB::XML::GEN_NAME)
      puts "added #{f} to container #{options['container']}"
   end
   txn.commit
end

      

      

