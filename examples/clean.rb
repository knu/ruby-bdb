require '../src/bdb'

class << BDB::Env
   def cleanup(dir, all = false)
      remove(dir) rescue nil
      Dir.foreach(dir) do |file| 
	 if FileTest.file?("#{dir}/#{file}")
	    File.unlink("#{dir}/#{file}") if all || /^log/ =~ file
	 end
      end
   end
end
