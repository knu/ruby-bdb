#!/usr/bin/ruby

def intern_def(text, names, fout)
   tt = text.join('#').gsub(/\(\(\|([^|]+)\|\)\)/, '<em>\\1</em>')
   tt = tt.gsub(/\(\(\{/, '<tt>').gsub(/\}\)\)/, '</tt>')
   tt.gsub!(/\(\(<([^>]+)>\)\)/, '<<???\\1???>>')
   fout.puts "##{tt}" 
   n = names[0]
   if n.sub!(/\{\s*\|([^|]+)\|[^}]*\}/, '')
      fout.puts "def #{n}yield #$1\nend"
   else
      fout.puts "def #{n}end"
   end
   if names.size > 1
      n = names[0].chomp.sub(/\(.*/, '')
      names[1 .. -1].each do |na|
	 na = na.chomp.sub(/\(.*/, '')
	 fout.puts "alias #{na} #{n}"
      end
   end
end

def output_def(text, names, keywords, fout)
   if ! names.empty?
      keywords.each do |k|
	 fout.puts k
	 intern_def(text, names, fout)
	 fout.puts "end" if k != ""
      end
   end
end

def loop_file(file, fout)
   text, keywords, names = [], [""], []
   comment = false
   rep, indent, vide = '', -1, nil
   IO.foreach(file) do |line|
      if /^#\^/ =~ line
	 comment = ! comment
	 next
      end
      if comment
	 fout.puts "# #{line}"
	 next
      end
      case line
      when /^\s*$/
	 vide = true
	 text.push line
      when /^#\^/
	 comment = ! comment
      when /^##/
	 line[0] = ?\s
	 fout.puts line
      when /^#\s*(.+?)\s*$/
	 keyword = $1
	 output_def(text, names, keywords, fout)
	 text, names = [], []
	 keywords = keyword.split(/\s*##\s*/)
	 if keywords.size == 1
	    fout.puts keywords[0]
	    keywords = [""]
	 end
      when /^#/
      when /^---/
	 name = $'
	 if vide
	    output_def(text, names, keywords, fout)
	    text, names = [], []
	    rep, indent, vide = '', -1, false
	 end
	 names.push name
      else
	 vide = false
	 if line.sub!(/^(\s*): /, '* ')
	    indent += ($1 <=> rep)
	    rep = $1
	 else
	    line.sub!(/^#{rep}/, '')
	 end
	 if indent >= 0
	    line = ('  ' * indent) + line
	 else
	    line.sub!(/\A\s*/, '')
	 end
	 text.push line
      end
   end
end

File.open("bdb.rb", "w") do |fout|
   loop_file('../bdb.rd', fout)
   Dir['*.rd'].each do |file|
      loop_file(file, fout)
   end
end
