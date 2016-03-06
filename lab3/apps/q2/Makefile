default:
	cd makeprocs; make
	cd generator_1; make
	cd generator_2; make
	cd reaction_1; make
	cd reaction_2; make
	cd reaction_3; make

clean:
	cd makeprocs; make clean
	cd generator_1; make
	cd generator_2; make
	cd reaction_1; make
	cd reaction_2; make
	cd reaction_3; make

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 3; ee469_fixterminal
