# GashaardUI
Gemaakt door Rick van Modem - Fluffy Media

Met een kleine dank aan Jerry Schun die mij m'n eerste Arduino opstuurde.

## Waarom heb ik dit gemaakt?
Dit script is mijn eerste script dat ik heb geschreven voor een ESP32/Arduino ding. Het is geboren uit pure noodzaak. De aansturing van onze oude gashaard was kapot en ik kon nergens een vervangende controller + afstandsbediening vinden.

## Wat heb ik voor hardware nodig?
* Een ESP32 (Bijv. WeMos of Geekreit van [Banggood](https://www.banggood.com/custlink/3vmECguVUU)\* / [AliExpress](https://s.click.aliexpress.com/e/_dYMXFWZ)\*)
* Een DC Motorcontroller (Bijv. een L298N van [Banggood](https://www.banggood.com/custlink/vGKRCeft2n)\* / [AliExpress](https://s.click.aliexpress.com/e/_dTC2QSl)\*)

\* Dit zijn affiliate links, je mag ook gewoon zelf naar Banggood of AliExpress browsen

## Waar gebruik je dit dus voor?
Nou wat het heel simpelgezegd is, is een apparaat waarmee je een motortje linksom of rechtsom kunt laten draaien voor een bepaalde tijd. Hiervoor gebruik ik een DC Motorcontroller die de '+' en '-' kan omdraaien en dus een motortje zo kan laten draaien (linksom of rechtsom). Hoe lang dit is, wordt uitgedrukt in milliseconden, maar de interface maakt het mogelijk om percentages te kiezen. Waarbij we een waarde vaststellen voor hoe lang draaien 100% is en vanuit daar dus een percentage omzetten naar een aantal milliseconden. Dus stel we stellen de 100% waarde in op 2000 ms draaien, dan is 50% dus 1000 ms draaien. Het aansturen hiervan gebeurt via een website die gehost wordt door een webserver op de ESP32/Arduino, of via Google Assistent -> IFTTT -> Adafruit -> MQTT -> ESP32/Arduino.

## Dus dit is alleen voor een gashaard?
Niet echt eigenlijk. Wat ik in dit script doe is een webserver aanmaken met een responsive interface, waarmee je verschillende functies kunt aansturen. Dat stuk zou je kunnen gebruiken voor iets anders. Ik laat zien hoe ik een commando vanaf Google Assistent verwerk, dat kun je gebruiken. En ik laat een motortje bewegen op basis van tijd/percentage. Dat zou je natuurlijk ook voor een kraan/pomp/druppelaar/weet ik het wat kunnen gebruiken.

## Waarom noem je het dan GashaardUI?
Ja weet ik veel.

## Welke libraries gebruik je?
* [PangolinMQTT](https://github.com/philbowles/PangolinMQTT)
* [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* Standaard ESP32 libraries voor Webserver en WIFI

## Hoe heb je dit gebouwd dan?
Ik heb veel uit de standaard examples en bij Pangolin bijgeleverde examples gehaald en sommige dingen via tutorials op YouTube en gewoon Gooloogoolen.

## Nog wat geleerd?
Ik kwam er al vrij snel achter dat delay(), hoewel veel gebruikt, echt een hatelijke functie is. Blijkbaar was dat algemene kennis, want nadat ik klaar was met mijn oplossing ervoor, werd ik doorgestuurd naar de 'Blink without delay' example. Blijkbaar waren er meer mensen.

De standaard MQTT library die iedereen gebruikte maakte ook gebruik van delay, waardoor mijn code niet meer werkte. Daarom heb ik deze vervangen met een asynchrone library [asyncMQTTClient](https://github.com/marvinroger/async-mqtt-client). Echter kwam ik er snel achter dat één gebruiker van die library het er echt niet mee eens was, omdat hij echt helemaal niet goed geprogrammeerd bleek te zijn. [Hier](https://github.com/marvinroger/async-mqtt-client/issues/193) lees je zijn grappige issue met de hele library. Hij heeft zelf de library gedeeltelijk herschreven, waar hij nog een paar keer uithaald naar hoe achterlijk die andere library wel niet gemaakt was. Arme Marvin Roger. Doet ie zo z'n best. Maar uiteindelijk toch maar voor de verbeterde library [PangolinMQTT](https://github.com/philbowles/PangolinMQTT) gegaan, Phil zal vast gelijk hebben.

## TODO's
* Ik wil de webserver ook asynchroon maken. Hier is een mooie library voor, die wil ik uiteindelijk gaan gebruiken. Om maar te zorgen dat die loop zo leeg mogelijk blijft.
* Optimaliseren. Ik heb na al mijn werk geleerd dat men niet heel blij is met String(). Iets waar ik mega veel gebruik van maak. Dit is niet echt netjes in C++ schijnbaar en dus wil ik kijken of ik dat misschien kan verbeteren.
* De bestanden die nu extern worden ingeladen op de webpagina's wil ik graag uiteindelijk vanaf de Arduino zelf inladen.
* Afbeeldingen worden nu als Base64 strings ingeladen, wat meer geheugen kost dan nodig.
* Datzelfde geldt voor de templates. Ik wil eigenlijk een template functie maken die een template als bestand kan inladen en dan bepaalde keywords kan  vervangen. Zo hoef ik niet van die lelijke string functies te gebruiken om de pagina html samen te stellen.