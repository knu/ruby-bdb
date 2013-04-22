# -*- coding: utf-8 -*-
require 'bundler/gem_tasks'

gemspec = Bundler::GemHelper.gemspec

require 'rake/extensiontask'
Rake::ExtensionTask.new('bdb', gemspec) do |ext|
  ext.cross_compile = true
  ext.cross_platform = 'x86-mingw32'
end

require 'rake/testtask'
Rake::TestTask.new(:test) do |test|
  test.libs << 'tests'
  test.test_files = gemspec.test_files
  test.verbose = true
end

task :test => :compile

task :default => :test
