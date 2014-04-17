import re
import sys

TIME_RE = re.compile("(CPU|Real) time used\s+=\s+(\S+)")
UPDATES_RE = re.compile(".*Number of updates[^=]+=\s+(\S+)")
GUPS_RE = re.compile("(\S+) Billion")

def main(argv):
	"""Parses the hpccinf.txt file given as command line argument.

	The output format is:
	ra[_<VIEW_IFDEF>] (Star|Single)(|LCG) <TAB> <GUP/s>
	ra[_<VIEW_IFDEF>] (Star|Single)(|LCG) <UP> <TAB TAB> <REAL_TIME> <USER_TIME>

	where (G)UP stands for (giga)updates.
	"""
	if len(argv) <= 1:
		sys.stderr.write("Usage: python %s HPCCINF_TXT [VIEW_IFDEF]\n" %
						 argv[0])
		return 1
	key = "ra" + ("_%s" % argv[2].lower() if len(argv) > 2 else "")

	section = None
	def parse(re_object):
		for l in section:
			m = re_object.match(l)
			if m:
				yield m.groups()

	for line in open(argv[1]):
		line = line.strip()
		if re.search("of (Single|Star)RandomAccess.*", line):
			if line.startswith("Begin"):
				name = re.search("(\S+RandomAccess\S*)", line).group(1)
				section = []
			elif line.startswith("End"):
				times = dict(parse(TIME_RE))
				update_cnt = list(parse(UPDATES_RE))[0][0]
				name = name.replace("RandomAccess", "")
				print "%s %-10s\t%s" % (key, name, list(parse(GUPS_RE))[0][0])
				print "%s %-10s %10s\t\t%s\t%s" % (
					key, name, update_cnt,
					times['Real'], times['CPU'])
				# print '\n'.join(section)
				section = None
		elif section is not None:
			section.append(line)
			# Check for update errors (possible if benchmark is modified/buggy)
			for m in parse(re.compile("Found (\d+) errors")):
				if int(m[0]):
					print >> sys.stderr, "Fatal: incorrect updates in", name
					return 2

if __name__ == '__main__':
	sys.exit(main(sys.argv))
