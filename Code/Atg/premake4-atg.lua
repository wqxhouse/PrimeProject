-- A project defines one build target
project("atg-".._OPTIONS["platformapi"])
	kind "StaticLib"
	language "C++"
	files { "**.h", "**.cpp" }