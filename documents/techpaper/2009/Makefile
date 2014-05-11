default: paper.pdf

SRC = abstract.tex conclusions.tex introduction.tex paper.tex electronics.tex sonar.tex vision.tex frame.tex  

paper.pdf: $(SRC)
	pdflatex paper

clean:
	rm -f paper.pdf paper.aux paper.log

.PHONY: default clean
