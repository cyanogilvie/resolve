#!/usr/bin/env tclsh

lappend auto_path	[file join [file dirname [file normalize [info script]]] ..]
package require resolve

resolve::resolver instvar lookup

$lookup add google.com:http
$lookup add www.rubylane.com:https -family AF_INET
$lookup add :ssh -protocol IPPROTO_TCP

$lookup go

# Do other things...

# Retrieve the results
puts "result for :ssh: [$lookup get :ssh]"
puts "result for www.rubylane.com:https: [$lookup get www.rubylane.com:https]"
puts "result for google.com: [$lookup get google.com:http]"

unset lookup
