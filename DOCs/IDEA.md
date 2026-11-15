Ideia conceitual: se basear no SMKSYNTH, e em outros para construir meu próprio firmware para o teclado controlador da mvave SMK37 PRO

A ideia desse software é: ser algo muito prático e não sofisticado na parte de synth, cheio de controles, quero dar ao usuário uma UX limpa e que ele tenha certeza o que está fazendo e que não se perca nos botões

Não vamos mexer na parte de SEQ, arpeggio, chord, global, isso pode ficar exatamente igual ao SMKSYNTH, por isso vamos nos basear nele

Do SMK Open podemos trazer a sintese FM

A ideia é do Clipper Pro apenas trazer alguns motores de sons (sampler, organ, oberhein, prophet e FM2 - buscar algum bom FM que seja distinto do FM presente no SMK open e JUNO) mas essa parte de engine sera bem secundaria, o foco central é performance ao vivo e alguns poucos tunnings

Por ex.

Quero manter pro modo Patch:

os 4 primeiros knobs mexem nos mesmos 4 parametros que no SMKSYNTH e me da o feedback visual disso

o ultimo knob (n. 8) sempre é o master volume (com a mesma regra do SMKSYNTH), os Knobs 5,6 e 7 sao knobs de navegacao igual ao SMKSYNTH mas vou especifica-los melhor abaixo

Os 4 Slider Faders:

- 4 FX Send controlando o quanto de envio pra aquele timbre estou mandando para chorus, delay, distortion e reverb

Modo Patch:

Linha 1 - Mostrar que estou no modo patch + Mostrar qual Bank do user estou (A,B,C,D) + qual dos 16 botoes de favoritos eu apertei caso tenha apertado algum
Linha 2 - Mostrar categoria de patches onde estou + nome do patch da lista selecionado

POr ex. [PATCH] - Bank A - 16
Pianos - 001 - Acoustic

> 001 - Acoustic
> 002 - Acoustic 2
> 003 - Supertramp
> 004 - DX7 EP 1
> 005 - DX7 EP 2

Ultima linha mostra a engine por ex DX Engine

O bank controla apenas os 16 botões de favoritos de patch

Se eu Clico e seguro um dos botões ele mostra o prompt pra confirmar se quero subscrever o timbre X que já estava favoritado ali

Ele me dá 5 segundos pra confirmar (confirmação é eu apertar o mesmo pad)

Caso salvo mostra "saved successfully" por 2 segundos e volta pra tela de patches

Caso não confirme o popup sai e volta pra tela de patches

Modo Performance:

Aqui teremos 14 PADs para favoritos e os 2 ultimos pads da ultima coluna
terao a funcao de Solar o timbre escolhido no SOLO 1 ou SOLO 2
a ideia é poder montar um patch de 2 Layers - 2 divisoes do teclado, e ter mais 2 patches que nunca tocam exceto se a gente apertar um daqueles pads da ultima
coluna, se eu apertar o ultimo de cima do canto superior eu entro no modo SOLO 1
e se apertar o ultimo de baixo do canto inferior direito entro no modo SOLO 2
se eu apertar de novo o mesmo PATCH saio do modo solo, se eu apertar qualquer outro PAD que nao sejam os ultimos 2 mas esteja no modo solo, eu nao mudo de favorito, primeiro eu saio do modo solo daquele FAV, apenas se eu apertar novamente ai sim navego entre qualquer um dos 14

A ideia é que possa tocar uma musica sofisticada que exija layer + mudanca em momentos distintos

Aqui gostaria que tivesse mais BANKs se a memoria permitir, dado que cada bank abriga apenas 14 combinacoes, idealmente de A a H - totalizando 8 Bancos de 14

1 Linha [Performance][BANK A-1]
Linha 2 [LAYER 1] 001 - BASS (C1-C3)
Linha 3 [LAYER 2] 157 - BRASS (C4-C7)
Linha 4 [SOLO 1] - 300 - LEAD
Linha 5 [SOLO 2] - 128 - MARIMBA

Como deu pra perceber eu quero que na Layer identifique entre paranteses o range de notas o qual aquele layer funciona

O modo de edicao do modo performance tem que ser muito facil e intuitivo, tem que ser facil escolher timbres e pensei em fazer da seguinte maneira de forma que a navegacao do teclado nunca mude e seja bem facil

Knob Bank -> Sempre alterna pro modo PATCH, clicar nele de forma repetida alterna entre os bancos A e D, os botoes de setinha nele navegam entre a categoria de timbres

Botao Fader Bank -> Sempre vai pro modo performance, clicar repetidas vezes alterna entre os bancos A e H

As setinhas aqui no modo edicao de Layer Solo abrem um popup de busca de patches o qual clicar nas setinhas alterna as categorias, com esse popup aberto o K1 passa a ser um navegador para navegar entre a lista de patches daquela categoria e o PAD 1 fica acesso azul para que seja confirmacao / selecao, o PAD 2 fica vermelho para que seja cancelamento e fechar a popup de selecao

Detalhe: a primeira categoria selecionavel é "FAVORITES" aqui tem sempre uma lista fixa dos 64 timbres favoritados no menu PATCH e só as proximas categorias sao PIANO / ORGAN / STRINGS/ etc

Apertar o KNOB BANK + FADER BANK no modo performance -> abre modo edit do performance mode, aqui só pode ser feita qualquer alteracao se entrar nesse modo, a primeira linha muda para:
[EDIT][Performance][A-16]

> [LAYER 1] F001 - Piano (C1-C7)
> [LAYER 2] -
> [SOLO 1] -
> [SOLO 2] -

Range: C1-C7. Volume: 99

Aparece um caractere > novo indicando que estou editando aquela linha

Caso eu escolhi um timbre da categoria favoritos, mostro F como prefixo do numero do timbre, caso contrario, sempre a numeracao do timbre que é imutavel

O Knob 1 vira o seletor de linha onde posso selecionar entre layer 1, layer 2, solo 1 e solo 2,
O knob 2 vira selecao, onde vou navegando entre todos timbres pra layer selecionada (lembrando que sempre esta disponivel nesse modo apertar KNOB BANK + FADER BANK pra fazer busca)
O knob 3 vira seletor inferior de oitava, indicando o range inferior de oitava daquele timbre
O knob 4 controla o volume daquele timbre (0-99) e ultimo dado que vamos mostrar na linha no modo edicao
O knob 5 controla se o sustain atua naquele timbre e na ultima linha a gente mostra SUSTAIN: YES ou NO (Default sempre YES)
O knob 6 controla o pitch bend, se esta ativo: YES ou NO (Default sempre YES)
O knob 7 diz o que o modulation faz para aquele timbre: DEFAULT, OFF ou algumas outras opcoes viaveis interessantes
O knob 8 por enquanto nao fazem nada

Os 16 pads no modo edicao tem um comportamento interessante:
Eles pre selecionam o primeiro timbre de cada categoria para a linha que estou selecionando, por ex, estou editando a linha SOLO 1, se aperto o PAD 1- ele atribiu o timbre F001 (primeiro timbre da categoria Favoritos naquela linha), se eu seleciono PAD 2 ele traz o primeiro timbre da categoria 02 - PIANO, minha ideia é nao passar de 16 categorias contando com a categoria favoritos:

01 - Favorites
02 - Acoustic Piano
03 - EP
04 - Organ
05 - Strings
06 - Wind
07 - Choir
08 - Brass
09 - Guitar
10 - Bass
11 - Pad
12 - Lead
13 - SFX
14 - Complex (ideia é colocar PADs e Leads arpejados)
15 - Drums - Use a sintese ja existente no SYNTH
16 - GM (General MIDI 128 - tentar usar a sintese FM pra atender todos, mesmo que alguns nao fiquem bons)

Por ex.

02 - Acoustic Piano (10)

JUNO Grand Piano
Grand Piano SAMP
Bright Grand SAMP
Mellow Grand
Ballad Grand
Pop Grand
Rock Grand
Upright Piano
FM Honky Tonk
SAMP Honky Tonk
FM Octave
FM M1
SAMP M1 Piano
FM Piano
FM Bright Piano

03 - EP (25)
Dyno EP
Suitcase EP
Rhodes Soft
Rhodes Bright
FM EP
DX EP 1
DX EP 2
CP80
EP Bell
Warm EP
Chorus EP
Phaser EP
Tremolo EP
Crunch EP
Vintage EP
Sparkle EP
Detuned EP
LoFi EP
EP Pad
EP Strings
Wurly Soft
Wurly Bite
Wurly Trem
Dirty EP
Bright Wurly

04 - Organ (25)

Purple Organ
Doors Organ
Jazz Organ
Rock Organ
Perc Organ
Full Drawbars
888000000
Gospel Organ
Church Organ
Pipe Organ
Cathedral Org
Baroque Organ
Theater Organ
Rotary Slow
Rotary Fast
Organ Perc
Dark Organ
Bright Organ
Vintage Organ
Transistor Org
Combo Organ
Farfisa
Vox Organ
Soft Organ
House Organ
Organ BassSplit
Organ Brass

05 - Strings
Slow Strings
Warm Strings
Stereo Strings
Chamber Str
Orch Strings
Legato Strings
Staccato Str
Pizzicato Str
Tremolo Str
Marcato Str

06 - Wind (25)

Flute
Ocarina
Pan Flute
Shakuhachi
Recorder
Piccolo
Alto Sax
Tenor Sax
Soprano Sax
Baritone Sax
Clarinet
Bass Clarinet
Oboe
English Horn
Bassoon
Harmonica
Blues Harp
Accordion
Musette Acc
Bandoneon
Whistle
Irish Flute
Ney Flute
Breathy Flute
Solo Sax
Wind Ensemble

07 - Choir

Choir Aah
Choir Ooh
Mixed Choir
Boys Choir
Female Choir
Male Choir
Gospel Choir
Classical Choir
Vocal Aah
Vocal Ooh
Voice Doo

08 - Brass (25)

Trumpet Solo
Muted Trumpet
Trombone Solo
Bass Trombone
French Horn
Horn Section
Brass Section
Bright Brass
Warm Brass
Pop Brass
Staccato Brass
Marcato Brass
Swell Brass
Synth Brass 1
Synth Brass 2

09 - Guitar

Steel Guitar
Nylon Guitar
Finger Guitar
Pick Guitar
12str Guitar
Jazz Guitar
Clean Guitar
Chorus Guitar
Muted Guitar
Funk Guitar
Overdrive Gtr
Distortion Gtr
Power Chord Gtr
Acoustic Strum
Ukulele
Mandolin
Banjo
Reso Guitar
Slide Guitar
Harmonics Gtr
Guitar Pad
Clean Octave Gtr
Jazz Box Gtr
PalmMute Guitar
LoFi Guitar

10 - Bass
Acoustic Bass
Finger Bass
Picked Bass
Fretless Bass
Slap Bass 1
Slap Bass 2
Pop Bass
Jazz Bass
Muted Bass
Flatwound Bass
Sub Bass
Synth Bass 1
Synth Bass 2
Moog Bass
Square Bass
Saw Bass
Acid Bass
Deep Bass
Punch Bass
Bass Pick
Octave Bass
Rubber Bass
FM Bass
Growl Bass
Upright Bass

11 - Pad
Warm Pad
Soft Pad
Analog Pad
Digital Pad
Air Pad
Choir Pad
String Pad
Sweep Pad
Slow Sweep
Dream Pad
Glass Pad
Dark Pad
Bright Pad
Space Pad
Cosmic Pad
Pulse Pad
Motion Pad
Shimmer Pad
Velvet Pad
Noise Pad
Detune Pad
Lush Pad
Epic Pad
Vintage Pad
LoFi Pad

12 - Lead
Saw Lead
Square Lead
Sync Lead
Mono Lead
Porta Lead
Fat Lead
Mini Lead
Moog Lead
Acid Lead
Soft Lead
Hard Lead
Vox Lead
Flute Lead
Reso Lead
PWM Lead
FM Lead
Pluck Lead
Bright Lead
Dark Lead
Unison Lead
Octave Lead
Whistle Lead
Solo Lead
Express Lead
Bend Lead

13 - SFX
Rise FX
Fall FX
Impact FX
Hit FX
Boom FX
Sweep Up
Sweep Down
Noise Burst
Reverse Cym
Vinyl Stop
Tape Stop
Laser FX
SciFi FX
Glitch FX
Spark FX
Wind FX
Thunder FX
Rain FX
Ocean FX
Helicopter
Explosion
Door Slam
UI Click
Robot Vox
Cinematic Hit

14 - Complex (Arp Pad/Lead)
ARP Warm 1
ARP Warm 2
ARP Bright 1
ARP Bright 2
ARP Pluck 1
ARP Pluck 2
ARP Saw Pulse
ARP Sync Lead
ARP Dream Pad
ARP Motion Pad
ARP Gate Pad
ARP Seq Bass
ARP Seq Lead
ARP Chord 1
ARP Chord 2
ARP Poly 1
ARP Poly 2
ARP Trance 1
ARP Trance 2
ARP Berlin
ARP Analog Seq
ARP Digital Seq
ARP Echo Lead
ARP Hybrid Pad
ARP Epic Seq

Minha ideia é usar 4 motores de sintese para atender os timbres,

engine FM para a maioria
engine SYNTH ja presente no SYNTH para varias coisas
engine SAMPLER para algumas pequenas coisas - por ex. poder disponibilizar um bom piano, uns sfx
engine ORGAN - para poder trazer uns timbres de organ mais sofisticados e profundos

os nomes dos timbres nao precisam ser exatamente esse e os timbres nao precisam ser todos da mesma engine, por ex. dentro de acoustic piano, posso ter coisa FM, coisa SYNTH e coisa SAMPLER, seria rico termos variacoes de mesmos timbres em motores distintos dando mais opcao ao usuario e identificando essas variacoes como coloquei acima na categoria Piano distinguindo pelos prefixos
FM, SYN, SP, ORG

se a gente quiser trazer sintese do Oberhein, Prophet, Moog podemos usar esses prefixos tambem

OBE, PRO, MOG, FM2 (algo que acharmos no clipper pro)

Importante a gente ter simulacao no browser

Instrucoes tecnicas:

pasta SMKSYNTH -> esse é o projeto base para nos basearmos 90% do trabalho para criar nosso firware, incluindo a parte do simulador web que deve ser gerado tambem

smk37_open -> esse é importante ler e documentar pois ele é o SDK oficial documentado pela fabricante do teclado
e aqui tem instrucoes importantes e boas praticas que talvez o firmware SMKSYNTH tenha deixado passar

clipper-smk -> aqui apenas vamos nos basear para trazer algunas engines adicionais para nosso projeto

vamos primeiro fazer uma rodada de grooming + spec e pode gerar uma pasta DOCs com specs e quebras de tudo que tem que
ser feito para atinginrmos nosso objetivo e tentando quebrar um pouco para que varios agents consigam fazer o trabalho
sem encher o contexto e ser continuavel caso queiramos fazer parte do trabalho aos poucos

- o codigo deve ser gerado numa nova pasta src
- devemos seguir as boas praticas dos firmwares pensando no risco de brick se a gente nao seguir
- devemos gerar uma parte web com simulador onde eu possa validar o que foi gerado antes da gente partir pro software
  final para o device real

- me pergunte qualquer detalhe importante que sinta que deixei passar
