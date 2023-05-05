# client-prediction
a simple implementation of client prediction and server reconciliation

Sadrzaj:
Na server se mogu spojiti max 20 clienta.
Moguće je postaviti "fake" latenciju (u svrhu testiranja).

Princip:
  - client pri povezivanju na server svoj tick stavlja u buducnost u odnosu na server
  - client se drzi 13 tickova ispred servera
  - client salje inpute serveru i sprema povijest inputa
  - server s delayem obradjuje primljene pakete i updatea svoj game state
  - server salje trenutni game state clientima
  - client prima game state od servera i usporedjuje svoj predicted state s serverovim
  - ako je client krivo predvidio preuzima serverov state
