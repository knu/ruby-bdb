#!/bin/sh
echo "=================================== query.rb"
ruby query.rb
echo "=================================== context.rb"
ruby context.rb
echo "=================================== document.rb"
ruby document.rb
echo "=================================== names.rb"
ruby names.rb
echo "=================================== metadata.rb"
ruby metadata.rb
echo "=================================== update.rb"
ruby update.rb
echo "=================================== delete.rb"
# ruby delete.rb
echo "=================================== add_index.rb"
ruby add_index.rb
echo "=================================== replace_index.rb"
ruby replace_index.rb
echo "=================================== delete_index.rb"
ruby delete_index.rb
echo "=================================== build_db.rb "
ruby build_db.rb 
echo "=================================== retrieve_db.rb"
ruby retrieve_db.rb
