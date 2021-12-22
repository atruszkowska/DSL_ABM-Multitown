#
# Script for removing files from tracking in bulk
#
# 	First run git status | grep token > git_st.log
#
# 	where token is a keyword characteristic for the
#	unwanted files
#

import subprocess

# If this keyword is in the file name, it will be removed
# This is a sanity check
token = 'dir_'

# Default name of file with names, see above
fname = 'git_st.log'

with open(fname, 'r') as fin:
	for line in fin:
		tracked_file = line.strip().split()[-1]
		if token in tracked_file:
			print(tracked_file)
			subprocess.call(['git update-index --skip-worktree ' + tracked_file], shell=True)	
