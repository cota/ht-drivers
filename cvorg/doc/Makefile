all : user_req

.PHONY : all clean user_req

user_req : user_req.tex
	pdflatex user_req.tex
	pdflatex user_req.tex

clean :
	rm -f *.aux *.out *.log .log *.pdf *.toc
