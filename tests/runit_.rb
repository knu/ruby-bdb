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
