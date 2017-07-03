import subprocess
#FNULL = open(os.devnull, 'w')    #use this if you want to suppress output to stdout from the subprocess
import csv
with open('tests - Copy.csv') as f:
    reader = csv.reader(f)
    for row in reader:
        args = ''
        for e in row:
            args = args +" " + e
        #filename = "One	None	First	N/A	InMiddle	MoreThanAllowed	None	None	Visits	Lifetime	Matches	AllUsedUp	ServiceIsCovered	NPAR	DoesNotMatter	InLast	Header	Valid	Invalid	FALSE	Override	Default	belowRange	TRUE	FALSE	Unlimited	N/A	TRUE	NonHMO	TRUE	DIA"
        args = "hcbug.exe " + str(args)
        p = subprocess.Popen(args,shell = True,stdout=subprocess.PIPE)
        output, err = p.communicate()
        print (output)
