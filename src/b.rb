require 'src/bdb'
db = BDB::Recno.open "marshal", nil, "w", "marshal" => Marshal
db[1] = Time.now
p db[1]
db.close
