require 'runit/testcase'
require 'runit/cui/testrunner'

module RUNIT
   module Assert
      def assert_error(error, string, message = nil)
	 begin
	    eval string
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
