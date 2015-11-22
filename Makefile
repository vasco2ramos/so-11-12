
SUBDIRS =  sthread_lib test-sthreads cinema test-cinema

#
# Dados sobre o grupo e turno frequentado 
# CAMPUS = preencher com A ou T consoante Alameda ou Tagus
# CURSO = indicar o curso do turno frequentado: LEIC ou LERC
# GRUPO = indicar o numero do grupo
# ALUNO1/ALUNO2/ALUNO3 = indicar os numeros dos alunos
#
CAMPUS=A
CURSO=LEIC
GRUPO=72
ALUNO1=62587
ALUNO2=67371
ALUNO3=69984

TOPO=pthreads
#TOPO=sthreads
ifeq ($(TOPO),sthreads)
DEFS= -m32 -DHAVE_CONFIG_H
else
DEFS= -m32 -DHAVE_CONFIG_H -DUSE_PTHREADS 
#DEFS= -DGC_DEBUG -DDEBUG -DC_DEBUG -DHAVE_CONFIG_H -DUSE_PTHREADS 
USE_PTHREADS=1
export USE_PTHREADS
endif

CFLAGS = -g -O0 -Wall -m32 -pthread
export DEFS
export CFLAGS

all: build

build:
	@list='$(SUBDIRS)'; for p in $$list; do \
	  echo "Building $$p"; \
	  $(MAKE) -C $$p; \
	done

clean:
	@list='$(SUBDIRS)'; for p in $$list; do \
	  echo "Cleaning $$p"; \
	  $(MAKE) clean -C $$p; \
	done

package: clean zip

zip:
ifndef CAMPUS
	@echo "ERROR: Must setup macro 'CAMPUS' correcly."
else
ifndef CURSO
	@echo "ERROR: Must setup macro 'CURSO' correcly."
else
ifndef GRUPO
	@echo "ERROR: Must setup macro 'GRUPO' correcly."
else
	tar -czf project-$(CAMPUS)-$(CURSO)-$(GRUPO)-$(ALUNO1)-$(ALUNO2)-$(ALUNO3).tgz * 
endif
endif
endif
