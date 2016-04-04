default:
	cd makeprocs; make
	cd hello_world; make
	cd q2_2; make
	cd q2_3; make
	cd q2_5; make
	cd q2_6; make

clean:
	cd makeprocs; make clean
	cd hello_world; make clean
	cd q2_2; make clean
	cd q2_3; make clean
	cd q2_5; make clean
	cd q2_6; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 1; ee469_fixterminal
	# cd ../../bin; dlxsim -D m -x os.dlx.obj -a -D m -u makeprocs.dlx.obj 1; ee469_fixterminal
