# smpt-client

Pour l'utiliser:
./client_SMTP xxx@he-arc.ch "Salut Roger" message localhost xxx@he-arc.ch

Pour simuler un serveur et envoyer des réponses spécifiques:
nc -l -p 12000
./client_SMTP xxx@he-arc.ch "Salut Roger" message localhost xxx@he-arc.ch 12000

serveurs à tester:
 - smtp.alphanet.ch
 - smtprel.he-arc.ch
