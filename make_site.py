import shutil
from os import listdir, system, mkdir, rmdir
from os.path import isfile, join, splitext
from distutils.dir_util import copy_tree

shutil.rmtree("generated")
mkdir("generated")
mkdir("generated/posts")
copy_tree("source/images", "generated/images")
copy_tree("source/style", "generated/style")
copy_tree("source/sourcecode", "generated/sourcecode")
shutil.copyfile("source/index.html", "generated/index.html")
shutil.copyfile("source/projects.html", "generated/projects.html")

post_path = "."
md_files = [f for f in listdir(post_path) if isfile(join(post_path, f)) and splitext(f)[1] == ".md"]
print(md_files)

for md_file in md_files:
	html_file = f"generated/posts/{splitext(md_file)[0]}.html"

	template_header, template_footer = open("source/post_template.html").read().split("@content")

	open(html_file, "a").write(template_header)
	system(f"smu {md_file} >> {html_file}")
	open(html_file, "a").write(template_footer)
