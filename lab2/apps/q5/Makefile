default:
	cd makeprocs; make
	cd reaction_1; make
	cd reaction_2; make
	cd reaction_3; make
	cd injection_h2o; make
	cd injection_so4; make

clean:
	cd makeprocs; make clean
	cd reaction_1; make clean
	cd reaction_2; make clean
	cd reaction_3; make clean
	cd injection_h2o; make clean
	cd injection_so4; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 3 3; ee469_fixterminal
