# general exception
class BDB::LockError < Exception
end

# Lock not held by locker
#
# inherit from BDB::LockError
class BDB::LockHeld < BDB::LockError
end

# Lock not granted
#
# inherit from BDB::LockError
class BDB::LockGranted < BDB::LockError
end

# Locker killed to resolve a deadlock
#
# inherit from BDB::LockError
class BDB::LockDead < BDB::LockError
end
