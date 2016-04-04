default:
	cd heaptest; make

clean:
	cd heaptest; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u heaptest.dlx.obj 10; ee469_fixterminal


runm:
	cd ../../bin; dlxsim -D m -x os.dlx.obj -a -D m -u heaptest.dlx.obj 10; ee469_fixterminal


runmp:
	cd ../../bin; dlxsim -D mp -x os.dlx.obj -a -D mp -u heaptest.dlx.obj 10; ee469_fixterminal

