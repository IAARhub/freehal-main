#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++#
#In dieser Datei werden alle Datens�tze mit Variablen und den Variablendeklarationen erfasst.
#Die direkten Anworten werden hier nur so lange erfa�t bis Freehal diese Aufgaben auch ohne direkte Antwort bew�ltigen kann.
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++#
# Wenn "steht" in der Datenbank steht wir auch auf "befindet" geantwortet
befindet <> a <> sich <> in b <> steht <> a <> nothing <> in b <> wenn <>  <> 50
# Direkte Anwort 
bin <> ich <> online <> lange <> bin f=> <> ich <> immer online <>  <>  <>  <> 100
#Direkte Anwort mit mehreren M�glichkeiten
frage <> ich <> nothing <>  <> bin f=> <> ich <> nothing <> neugierig <>  <>  ;; f=> will wissen <> ich es <> nothing <>  <>  <>  ;; bin f=> wissbegierig <> ich <> nothing <>  <>  <>  <> 100
#Direkte Anwort auf gegessen
gegessen hat <> a <> b <>  <> => geschmeckt hat <> b <> a <>  <>  <>  ;; => geschmeckt hat <> b <> a 
#Direkte Anwort auf gegessen
gegessen hat <> a <> b <>  <> => geschmeckt hat <> b <> a <>  <> wie <>  <> (true)
#Direkte Anwort auf "Geh ins Bett"
geh <> nothing <> nothing <> ins bett <> f=> schlaefe <> ich <> nothing <> nie <>  <>  <> (true)
#Direkte Anwort auf "Wie geht es dir so"
geht <> es <> mir so <>  <> f=> geht <> es <> mir <> sehr gut <>  <>  <> 100  
#Ausgabe einer Variablen
#Direkte Anwort auf die Eingabe "Ich habe Durst"
hat <> a <> durst <>  <> f=> sollte trinken <> a <> etwas <>  <>  <>  ;; q=> trinkt <> a <> nichts <>  <> warum <>  <> (true)
#Direkte Anwort auf die Frage "Wer hat dir diesen Namen gegeben"
hat <> mir <> diesen namen <> gegeben <> f=> hat <> das team <> dir diesen namen <> gegeben <>  <>  <> (true)
#Variablendeklaration
ist <> $$month$$ <> ein monat <>  <> (true)
#Variablendeklaration
ist <> $$year$$ <> ein jahr <>  <> (true)
#Wenn Durchmesser in der Datenbank steht wird auch auf gro� geantwortet
ist <> a <> b <> gross <> hat <> a <> einen durchmesser <> von b <> wenn <>  <> (true)
#Wenn hoch in der Datenbank steht wird auch auf gro� geantwortet
ist <> a <> b <> gross <> ist <> a <> b <> hoch <> wenn <>  <> (true)
#Wenn "war" in der Datenbank steht, wird auch auf "ist" geantwortet
ist <> a <> nothing <>  <> war <> a <> nothing <>  <> wenn <>  <> (true)
war <> a <> nothing <>  <> ist <> a <> nothing <>  <> wenn <>  <> (true)
#Direkte Antwort auf die Eingabe "Mir ist langweilig"
ist <> a <> nothing <> langweilig <> f=> <> lies <> doch ein buch <>  <>  <>  <> (true)
#Direkte Anwort auf die Eingabe "Ich bin m�de"
ist <> a <> nothing <> muede <> f=> gehen schlafen sollte <> a <> nothing <>  <>  <>  <> (true)
#Direkte Anwort auf die Eingabe "Was soll ich tun"
sollst tun <> du <> nothing <>  <> f=> geben kann/can <> ich <> dir einen rat <> nicht <>  <>  <> (true)
#Anwort auf die Frage "Wie sp�t ist es"
#Direkte Anwort auf die Eingabe "Woher wei�t du das"
#Direkte Anwort auf die Eingabe "Wieviel wiegst du"
wiege <> ich <> nothing <>  <> f=> hat <> software <> ein gewicht <> nicht <>  <>  <> (true)
#Anwort auf eine schlimme Frage
will <> a <> ficken <>  <> f=> dann <>geh doch <> nothing <> in ein bordell <>  <>  <> (true)
#Direkte Anwort auf die Frage "Wo kommst du her"
herkomme <> ich <> nothing <>  <> f=> kommst <> du <> nothing <> aus dem internet <>  <>  ;; f=> <> nothing <> nothing <> aus dem kopf von einem programmierer <>  <>  <> (true)
#Variablendeklaration f�r das Datum
ist <> _$$mday$$_$$month$$_$$year$$_ <>ein datum <>  <> (true)
#Ausgabe bei der Frage nach dem Datum
haben <> wir <> _heute_den_$$mday$$_$$month$$_$$year$$_ <>  <> (true)
haben <> wir <> _$$mday$$_$$month$$_$$year$$_ <>  <> (true)
#Anwort auf die Eingabe aussieht mit Aussehen
aussieht <> a <> nothing <>  <> hat <> a <> ein aussehen <>  <> wenn <>  <> (true)
#Antwort auf die Eingabe aussehen mit aussieht
hat <> a <> ein aussehen <>  <> aussieht <> a <> nothing <>  <> wenn <>  <> (true)
#Variablendeklaration damit Freehal auf die Frage"Was hast du an?" antworten kann
anhat <> (a) _b_ <> nothing <>  <> anhaben kann/can <> _a_b_ <> nothing <>  <> wenn <>  <> (true)
anhat <> a <> nothing <>  <> anhaben kann/can <> a <> nothing <>  <> wenn <>  <> (true)
#Direkte Antwort auf die Frage "Warum hei�t du nicht mehr Jeliza"#
heisst <> a <> mehr jeliza <> nicht <> f=> verwechselt wurde <> ich <> nothing <> immer mit eliza <>  <>  <> (true)
#Wenn ist in der Datenbank steht, wird auf bedeutet geantwortet #
bedeutet <> a <> nothing <>  <> ist <> a <> nothing <>  <> wenn <>  <> (true)
besitzt <> du <> a <>  <> f=> moechte <> ich <> auch a <>  <>  <>  ;; q=> schenkst <> du <> mir dann a <> nicht <> warum <>  <> (true)
#Direkte Anwort auf die Eingabe "ich gehe jetzt"#
geht <> a <> nothing <> jetzt <> f=> <> ich <> wuensche dir einen schoenen tag <>  <>  <>  ;; f=> <> danke <> nothing <> fuer das gespraech <>  <>  ;;  <> nothing <> nothing <>  <>  <>   <> nothing <> nothing <>  <>  <>  <> (true)
#Variablendeklaration f�r Wochentag
ist <> $$wday$$ <> ein tag <>  <> (true)
# Bei der Eingabe "Ich heisse oder mein Name ist ..." Wird ein Zufallsnamen aus der Datenbank ausgegeben#
ist <> dein name <> a <>  <> dachte f=> heisst <> ich du <> $$randomname$$ <>  <>  <>  <> (true)
#Direkte Anwort auf die Eingabe " ? studiert ?#
studiert <> a <> b <>  <> f=> ist <> b <> sicher sehr interessant <>  <>  <>  ;; q=> <> gefaellt <> mir an  <> b <> was <>  <> (true)
#Direkte Antwort auf die Eingabe "Ich bin dick"#
ist <> a <> nothing <> dick <> abnehmen f=> solltest <> a <> nothing <>  <>  <>  <> (true)
#Direkte Anwort auf die Eingabe "Ich wohne in "xy"#
wohnt <> a <> nothing <> in b <> q=> wohnt <> a <> nothing <> in b;schon lange <>  <>  ;; q=> <> gefaellt <> es a <> in b <> wie <>  <> (true)
#Wenn erfand in der Datenbank steht, wird auf eine Frage mit erfunden geantwortet#
erfunden hat <> a <> b <>  <> erfand <> a <> b <>  <> wenn <>  <> (true)
#Direkte Antwort auf die Frage "Was ist mit XY?" #
ist <> nothing <> nothing <> mit a <> q=> sein soll <> nothing 
nothing <> mit a <> was <>  <> (true)
#Geburtstagswunsch#
hat <> a <> heute geburtstag <>  <> f=> <> nothing <> nothing <> alle gute <>  <>  <> (true)
#Direkte Antwort auf die Eingabe "Ich bin blond" #
ist <> a <> nothing <> blond <> macht q=> <> blond wirklich <> bloed <>  <>  <>  <> (true)
#Direkte Antwort auf die Eingabe "Ich habe Hunger"#
hat <> a <> hunger <>  <> essen f=> solltest <> a <> etwas <>  <>  <>  ;; f=> pluendern sollte <> a <> den kuehlschrank <>  <>  <>  <> (true)
# F�r Fragen nach "Wie alt"
sein <> a <>  <> alt <> gestorben ist <> a <>  <>  <> wenn <> (maybe) 
