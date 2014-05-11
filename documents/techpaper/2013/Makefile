default: paper.pdf

SRC = *.tex fig/*

paper.pdf: $(SRC)
	pdflatex paper
	# Need to run twice to get references to work on my machine - Victor
	pdflatex paper

clean:
	rm -f paper.pdf paper.aux paper.log

.PHONY: default clean
