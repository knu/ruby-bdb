begin
   require 'test/unit'
rescue LoadError
   require 'runit/testcase'
   require 'runit/cui/testrunner'

   module RUNIT
      module Assert
	 def assert_raises(error, message = nil)
	    begin
	       yield
	    rescue error
	       assert(true, message)
	    rescue
	       assert_fail("must fail with #{error} : #{string}")
	    else
	       assert_fail("*must* fail : #{string}")
	    end
	 end
      end
   end
end


if RUBY_VERSION >= "1.8"
   class Array
      alias indices values_at
   end
   class Hash
      alias indexes values_at
   end
   module BDB
      class Common
	 alias indexes values_at
      end

      class Recnum
	 alias indices values_at
      end
   end
end
