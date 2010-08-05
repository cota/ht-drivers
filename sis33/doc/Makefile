.PHONY: all clean
.SUFFIXES: .pdf .tex
.tex.pdf:
	pdflatex $*.tex
	pdflatex $*.tex

all: user_req.pdf sines.pdf
clean:
	rm -f *.aux *.out *.log .log *.pdf
