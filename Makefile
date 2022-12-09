makeproject: cs301project.cpp
	g++ -std=c++11 cs301project.cpp -o a.out

clean:
	rm -f a.out