# GashaardUI
Gemaakt door Rick van Modem - Fluffy Media

## Waarom heb ik dit gemaakt?
Dit script is mijn eerste script dat ik heb geschreven voor een ESP32/Arduino ding. Het is geboren uit pure noodzaak. De aansturing van onze oude gashaard was kapot en ik kon nergens een vervangende controller + afstandsbediening vinden.

## Waar gebruik je dit dus voor?
Nou wat het heel simpelgezegd is, is een apparaat waarmee je een motortje linksom of rechtsom kunt laten draaien voor een bepaalde tijd. Hiervoor gebruik ik een DC Motorcontroller die de '+' en '-' kan omdraaien en dus een motortje zo kan laten draaien (linksom of rechtsom). Hoe lang dit is, wordt uitgedrukt in milliseconden, maar de interface maakt het mogelijk om percentages te kiezen. Waarbij we een waarde vaststellen voor hoe lang draaien 100% is en vanuit daar dus een percentage omzetten naar een aantal milliseconden. Dus stel we stellen de 100% waarde in op 2000 ms draaien, dan is 50% dus 1000 ms draaien. Het aansturen hiervan gebeurt via een website die gehost wordt door een webserver op de ESP32/Arduino, of via Google Assistent -> IFTTT -> Adafruit -> MQTT -> ESP32/Arduino.

## Hoe heb je dit gebouwd dan?
Ik heb veel uit de standaard Examples gehaald en sommige dingen via tutorials op YouTube en gewoon Gooloogoolen.

## Nog wat geleerd?
Ik kwam er al vrij snel achter dat delay(), hoewel veel gebruikt, echt een hatelijke functie is. Blijkbaar was dat algemene kennis, want nadat ik klaar was met mijn oplossing ervoor, werd ik doorgestuurd naar de 'Blink without delay' example. Blijkbaar waren er meer mensen.

De standaard MQTT library die iedereen gebruikte maakte ook gebruik van delay, waardoor mijn code niet meer werkte. Daarom heb ik deze vervangen met een asynchrone library. Echter kwam ik er snel achter dat één gebruiker van die library het er echt niet mee eens was, omdat hij echt helemaal niet goed geprogrammeerd bleek te zijn. Hij heeft zelf de library gedeeltelijk herschreven.

## TODO's
- Ik wil de webserver ook asynchroon maken. Hier is een mooie library voor, die wil ik uiteindelijk gaan gebruiken. Om maar te zorgen dat die loop zo leeg mogelijk blijft.
- Optimaliseren. Ik heb na al mijn werk geleerd dat men niet heel blij is met String(). Iets waar ik mega veel gebruik van maak. Dit is niet echt netjes in C++ schijnbaar en dus wil ik kijken of ik dat misschien kan verbeteren.
- De bestanden die nu extern worden ingeladen op de webpagina's wil ik graag uiteindelijk vanaf de Arduino zelf inladen.
- Afbeeldingen worden nu als Base64 strings ingeladen, wat meer geheugen kost dan nodig.
- Datzelfde geldt voor de templates. Ik wil eigenlijk een template functie maken die een template als bestand kan inladen en dan bepaalde keywords kan  vervangen. Zo hoef ik niet van die lelijke string functies te gebruiken om de pagina html samen te stellen.