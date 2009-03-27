#!/usr/bin/env ruby

$LOAD_PATH << '../build'

require 'despotify'
require 'pp'

username = ARGV.shift
password = ARGV.shift

if not (username and password)
	puts 'Need username & password'
	exit
end

begin
	despotify = Despotify::Session.new(username, password)
rescue Despotify::DespotifyError
	puts 'Failed to authenticate user'
	exit
end

artist = Despotify::Artist.new(despotify, '691a84294bfb4883a2124099bf1d0a8c')

pp artist
pp artist.name
pp artist.metadata

albums = artist.albums

albums.each do |album|
	pp album.name
	pp album.metadata
#	album.tracks.each do |track|
#		pp track.metadata
#	end
end
