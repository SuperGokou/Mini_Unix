default: my-cat my-uniq my-sed

my-cat: my-cat.c
	gcc -o my-cat my-cat.c -Wall -Werror

my-sed: my-sed.c
	gcc -o my-sed my-sed.c -Wall -Werror

my-uniq: my-uniq.c
	gcc -o my-uniq my-uniq.c -Wall -Werror

testmc: my-cat
	./my-cat sample.txt

testmu: my-uniq
	@./my-uniq sample.txt

clean: my-uniq my-cat my-sed
	rm my-uniq my-cat my-sed
