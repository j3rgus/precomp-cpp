import sys
import subprocess
from os import walk

def test_files(prog, directory, files):
	for filename in files:
		cmd = './' + prog + ' ' + directory + filename
		proc = subprocess.call(cmd.split(), stderr=open('/dev/null', 'wb'), close_fds=True)
		if proc == 0:
			print(filename + ' prased properly')
		else:
			print(filename + ' failed to parse')

if __name__=='__main__':
	if len(sys.argv) != 3:
		print("Usage: ./test_samples.py <prog_name> <directory>")
		sys.exit(1)

	_, _, files = next(walk(sys.argv[2]), (None, None, []))
	test_files(sys.argv[1], sys.argv[2], files)
