bdb1
====

Synopsis
--------

A Ruby interface to Berkeley DB distributed by Oracle

Prerequisite
============

* db >= 2 (some functionnality like join are not available with db < 2.6)

For Berkeley DB 1.85 and 1.86 see bdb1

Examples
--------

See the `examples` directory for code examples.

Installation
------------

You can install this module simply by:

	gem install bdb

Use the `--with-db-dir=$prefix` option to specify with which libdb
this extension should be linked.

Notes
=====

With bdb >= 0.5.5 `nil' is stored as an empty string (when marshal is
not used).

Open the database with

    "store_nil_as_null" => true

if you want the old behavior (`nil' stored as `\000').

Examples
========

* examples/basic.rb

  simple access method

* examples/recno.rb

  access to flat file

* examples/cursor.rb

  direct cursor access

* examples/txn.rb

  transaction

* examples/join.rb

  join (need db >= 2.6)

* examples/log.rb

  log file

License
-------

Copyright (c) 2000-2008 Guy Decoux
Copyright (c) 2008-2011 Akinori MUSHA

You can redistribute it and/or modify it under the same term as Ruby.
