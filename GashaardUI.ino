/* --------------------------------------------- */
/*  Gashaard via webserver en Google Assistent   */
/* --------------------------------------------- */

// Gemaakt door Rick van Modem - Fluffy Media

// Dit script is mijn eerste script dat ik heb
// geschreven voor een ESP32/Arduino ding. Het is
// geboren uit pure noodzaak. De aansturing van 
// onze oude gashaard was kapot en ik kon nergens
// een vervangende controller + afstandsbediening
// vinden.
//
// Ik heb veel uit de standaard Examples gehaald
// en sommige dingen via tutorials op YouTube en
// gewoon Gooloogoolen.
//
// Ik kwam er al vrij snel achter dat delay(),
// hoewel veel gebruikt, echt een hatelijke functie
// is. Blijkbaar was dat algemene kennis, want
// nadat ik klaar was met mijn oplossing ervoor,
// werd ik doorgestuurd naar de 'Blink without delay'
// example. Blijkbaar waren er meer mensen.
//
// De standaard MQTT library die iedereen gebruikte
// maakte ook gebruik van delay, waardoor mijn
// code niet meer werkte. Daarom heb ik deze ver-
// vangen met een asynchrone library. Echter kwam
// ik er snel achter dat één gebruiker van die library
// het er echt niet mee eens was, omdat hij echt
// helemaal niet goed geprogrammeerd bleek te zijn.
// Hij heeft zelf de library gedeeltelijk herschreven.
//
// TODO:
// - Ik wil de webserver ook asynchroon maken. Hier
//   is een mooie library voor, die wil ik uiteindelijk
//   gaan gebruiken. Om maar te zorgen dat die loop
//   zo leeg mogelijk blijft.
// - Optimaliseren. Ik heb na al mijn werk geleerd dat
//   men niet heel blij is met String(). Iets waar ik
//   mega veel gebruik van maak. Dit is niet echt netjes
//   in C++ schijnbaar en dus wil ik kijken of ik dat
//   misschien kan verbeteren.
// - De bestanden die nu extern worden ingeladen op de
//   webpagina's wil ik graag uiteindelijk vanaf de 
//   Arduino zelf inladen.
// - Datzelfde geldt voor de templates. Ik wil eigenlijk
//   een template functie maken die een template als
//   bestand kan inladen en dan bepaalde keywords kan
//   vervangen. Zo hoef ik niet van die lelijke string
//   functies te gebruiken om de pagina html samen te
//   stellen.

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// We maken gebruik van de PangolinMQTT library 
// https://github.com/philbowles/PangolinMQTT
// Deze library is ontstaan doordat de developer ervan
// het zat was dat de library die hij gebruikte niet
// stabiel was. Later ontdekte hij dat die andere
// library eigenlijk alleen maar per ongeluk werkte
#include <PangolinMQTT.h>

// Globale definities

// De koppeling gaat nu als volgt:
// Google Assistant -> IFTTT.com -> io.adafruit.com
// Deze gegevens maak je aan op io.adafruit.com
const char* aioServer = "io.adafruit.com";
const int aioPort = 1883;
const char* aioUsername = "";
const char* aioKey = "";
// Een adafruit endpoint is altijd "{Jouw Username}/feeds/{Naam van je Adafruit project"}
const char* aioEndPoint = "";

// De gegevens voor het netwerk voer je hier in
// ssid en password zijn voor het WIFI netwerk
// domain is de naam van de local domain (zonder .local).
// Als je dit als url wilt voor je webserver: http://arduino.local/
// dan vul je bij domain = "arduino" in.
const char* ssid = "";
const char* password = "";
const char* domain = "";

// We gebruiken deze twee pins voor het aansturen van
// de DC motor controller
const int pinOpening = 26;
const int pinClosing = 18;

PangolinMQTT mqttClient;

// We gebruiken ook de default webserver die standaard
// bij de ESP32 libraries zit.
WebServer server(80);

// Deze debug timer gebruiken we om te kijken of, als
// we verwachten dat de motor 2000 ms aan staat, deze
// ook daadwerkelijk 2000ms aan staat. Door delays in 
// verschillende libraries kon dit namelijk fluctueren.
// Omdat we nu MQTT asynchroon gebruiken, is dat
// probleem verholpen gelukkig.
int debugTimer = 0;

// Dit is de default waarde voor het volledig openzetten
// van de gaskraan. Van helemaal uit tot 'volledig' aan.
int timeUntilFull = 6000;
// Dit is de tijd die het duurt voordat het gas voor het
// eerst ontbrandt. Deze 2000 ms gebeurt er dus nog niets
// vanaf volledig uit.
int timeDeadzone = 2000;
// Globale variabele die gebruikt wordt voor het instellen
// van 'timeUntilFull' via de startTimer() functie
int tempTime = 0;
// We gaan er vanuit dat als dit script gestart wordt, dat
// de gaskraan volledig gesloten is. Het percentage is 0.
int currentPercentage = 0;
// We beginnen met beide pins op LOW. Met andere woorden,
// we willen niet dat het script bij het starten meteen 
// gaat proberen de gaskraan te openen of te sluiten.
// Ook is hij bij het starten van het script niet 'bezig'
// met openen of sluiten.
int pinOpeningState = LOW;
int pinClosingState = LOW;
bool isOpening = false;
bool isClosing = false;

// Deze variabele wordt gebruikt voor het bepalen van hoe
// lang de gaskraan open gedraaid of gesloten moet worden.
// De loop() functie kan hierdoor zonder delay() de kraan
// voor een bepaalde tijd openen of sluiten.
int timeToMove = 0;
// Deze variabele wordt gezet op het moment dat de gaskraan
// moet openen of sluiten en wordt gebruikt door de loop()
// functie om te bepalen hoe lang de open- of sluitactie al
// bezig is.
int timeToMoveStart = 0;

// De functie die de bovenkant van de HTML pagina als String
// geeft. Zodat elke pagina dezelfde bovenkant kan gebruiken
// en we geen herhalende code krijgen.
//
// TODO: Ooit omzetten in een template systeem dat gebruik
// maakt van bestanden.
String getHeader(String title = "Gashaard") {
    // Voor style en scripts maken we gebruik van externe CDN's.
    // Op die manier hoeven we de bestanden niet op de aruino op
    // te slaan.
    // De afbeelding/icon wordt als BASE64 string in de code
    // gezet. Dit heeft als nadeel dat hij groter is dan zou
    // hoeven, als we hem als bestand hadden gebruikt.
    //
    // TODO: Ooit omzetten naar dat de bestanden vanaf de arduino
    // komen. Dan is er voor de standaard functionaliteit geen
    // internetverbinding nodig.
   
    String htmlHeader = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><title>";
    htmlHeader += title;
    htmlHeader += "</title>";
    htmlHeader += "<link rel=\"apple-touch-icon\" sizes=\"128x128\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIAAAACACAYAAADDPmHLAAAACXBIWXMAAAsTAAALEwEAmpwYAAAFFmlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgNi4wLWMwMDIgNzkuMTY0NDYwLCAyMDIwLzA1LzEyLTE2OjA0OjE3ICAgICAgICAiPiA8cmRmOlJERiB4bWxuczpyZGY9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkvMDIvMjItcmRmLXN5bnRheC1ucyMiPiA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1sbnM6cGhvdG9zaG9wPSJodHRwOi8vbnMuYWRvYmUuY29tL3Bob3Rvc2hvcC8xLjAvIiB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIgeG1sbnM6c3RFdnQ9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZUV2ZW50IyIgeG1wOkNyZWF0b3JUb29sPSJBZG9iZSBQaG90b3Nob3AgMjEuMiAoV2luZG93cykiIHhtcDpDcmVhdGVEYXRlPSIyMDIwLTA4LTI5VDE2OjIyOjU0KzAyOjAwIiB4bXA6TW9kaWZ5RGF0ZT0iMjAyMC0wOC0yOVQxNjoyMzoxOCswMjowMCIgeG1wOk1ldGFkYXRhRGF0ZT0iMjAyMC0wOC0yOVQxNjoyMzoxOCswMjowMCIgZGM6Zm9ybWF0PSJpbWFnZS9wbmciIHBob3Rvc2hvcDpDb2xvck1vZGU9IjMiIHBob3Rvc2hvcDpJQ0NQcm9maWxlPSJzUkdCIElFQzYxOTY2LTIuMSIgeG1wTU06SW5zdGFuY2VJRD0ieG1wLmlpZDo2ZWZmY2UxOC0wMTBjLTdk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\"><link rel=\"icon\" sizes=\"192x192\" href=\"data:image/png;base64,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\">";
    htmlHeader += "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css\" integrity=\"sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z\" crossorigin=\"anonymous\"><link rel=\"stylesheet\" href=\"https://pro.fontawesome.com/releases/v5.10.0/css/all.css\" integrity=\"sha384-AYmEC3Yw5cVb3ZcuHtOA93w35dYTsvhLPVnYs9eStHfGJvOvKxVfELGroGkvsg+p\" crossorigin=\"anonymous\"/><style type=\"text/css\">h1 {margin-bottom: 0;margin-top: 30px;}.btn {width: 100%;}.col {margin-bottom: 30px;}</style></head><body><div class=\"container\">";
    return htmlHeader;
}

// De functie die de onderkant van de HTML pagina als String
// geeft. Zodat elke pagina dezelfde onderkant kan gebruiken
// en we geen herhalende code krijgen.
//
// TODO: Ooit omzetten in een template systeem dat gebruik
// maakt van bestanden.
String getFooter() {
    String htmlFooter = "</div></body></html>";
    return htmlFooter;
}

// Functie die de header en footer combineert met html uit
// een String
String getHtml(String content, String title) {
    String html;
    // Haal de bovenkant van de webpagina op
    html = getHeader(title);

    // Voeg de content toe
    html += content;

    // Voeg de footer toe
    html += getFooter();
    return html;
}

// Afhandeling van de url '/'. Op deze pagina staan alle
// knoppen om de gaskachel te bedienen en een knop naar het
// instellingen menu.
void handleRoot() {
    String content = "<div class=\"row\"><div class=\"col col-12 text-center\"><h1>Gashaard</h1></div></div><div class=\"row\">";
    String btnOffClass = "btn-secondary";
    // Als het percentage groter is dan 0 (de gaskraan staat dus open)
    // maak dan de class van de 'uit button' zo dat deze rood gekleurd is.
    if (currentPercentage > 0)
    {
    btnOffClass = "btn-danger";
    }
    content += "<div class=\"col col-12\"><a href=\"/off\" class=\"btn "+btnOffClass+" btn-lg\"><i class=\"fa fa-fire-extinguisher\"></i><br>Uit</a></div>";

    // Zet 10 knoppen op het scherm in stappen van 10. Als de huidige
    // button de waarde heeft van het huidige percentage, geef die button
    // dan een class die de button groen maakt.
    for (byte i = 10; i <= 100; i = i + 10) {
    String btnClass = "btn-primary";
    if (i == currentPercentage)
    {
        btnClass = "btn-success";
    }
    content += "<div class=\"col col-6\"><a href=\"/fire?p="+String(i)+"\" class=\"btn "+btnClass+" btn-lg\"><i class=\"fa fa-fire-alt\"></i><br>"+String(i)+"%</a></div>";
    }
  
    content += "<div class=\"col col-12\"><a href=\"/settings\" class=\"btn btn-secondary btn-lg\"><i class=\"fa fa-cog\"></i><br>Instellingen</a></div></div>";
    // Stuur de html pagina naar de browser. Vorm de response.
    server.send(200, "text/html", getHtml(content, "Gashaard"));
}

// Afhandeling van de url '/settings'. Op deze pagina kunnen verschillende
// zaken worden afgelezen, zoals het ip-adres en de huidige instellingen.
// Ook kun je vanuit hier de timer reset starten en de deadzone tijd aanpassen
void handleSettings() {
    String content = "<div class=\"row\"><div class=\"col col-2\"><a href=\"/\" class=\"btn btn-primary mt-3\"><i class=\"fa fa-undo\"></i></a></div><div class=\"col col-8 text-center\"><h1>Instellingen</h1></div><div class=\"col col-2\"></div></div><div class=\"row\"><div class=\"col col-12\"><div class=\"card\"><div class=\"card-header text-center\"><strong>Huidige gegevens</strong></div><div class=\"card-body p-0\"><table class=\"table m-0\">";
    content += "<tr><th>IP-Adres</th><td>" + String(WiFi.localIP().toString()) + "</td></tr>";
    content += "<tr><th>Tijd van 0 tot 100%</th><td>" + String(timeUntilFull) + " ms</td></tr>";
    content += "<tr><th>Deadzone</th><td>" + String(timeDeadzone) + " ms</td></tr>"; 
    content += "</table></div></div></div></div><div class=\"row\"><div class=\"col col-12\"><a href=\"/settings/startTimer\" class=\"btn btn-primary btn-lg\" onclick=\"return confirm('Weet je zeker dat je de timer wilt resetten?');\"><i class=\"fa fa-clock\"></i><br>Reset timer</a></div></div><div class=\"row\"><div class=\"col col-12\"><form method=\"GET\" action=\"/settings/deadzone\"><div class=\"form-group\"><label for=\"deadzone\">Deadzone</label><input type=\"number\" min=\"0\" max=\""+String(timeUntilFull)+"\" step=\"100\" class=\"form-control\" id=\"deadzone\" placeholder=\"Bijvoorbeeld 2000 voor 2 seconden\" name=\"deadzone\" value=\""+timeDeadzone+"\"></div><div class=\"form-group\"><button class=\"btn btn-primary btn-lg\"><i class=\"fa fa-save\"></i><br>Deadzone opslaan</button></div></form></div></div>";

    server.send(200, "text/html", getHtml(content, "Instellingen"));
}

// Afhandeling van de url /settings/deadzone?deadzone=<NIEUWE WAARDE IN MS>
// Het aanpassen van de deadzone gebeurt via een GET url met de deadzone parameter.
void handleDeadzone() {
    String deadzone = server.arg("deadzone");
    // De parameter is een string (of in ieder geval geen int), dus zetten we
    // hem om naar een int.
    timeDeadzone = deadzone.toInt();
    redirect("/settings");
}

// De instelling voor 'timeUntilFull' kun je aanpassen
// door de gaskraan helemaal dicht te draaien. Daarna start
// je een timer vanuit de instellingen. Als de gaskraan
// volgens jou op 100% staat, stop je de timer. 
void handleStartTimer() {
    // Sla NU op voor de loop() functie
    tempTime = millis();
    // Zet de gaskraan op openen. De gaskraan 
    digitalWrite(pinOpening, HIGH);

    String content = "<div class=\"row\"><div class=\"col col-12 text-center\"><h1>Instellingen</h1></div></div><div class=\"row\"><div class=\"col col-12\"><a href=\"/settings/stopTimer\" class=\"btn btn-primary btn-lg\"><i class=\"fa fa-clock\"></i><br>Stop timer</a></div></div>";
    server.send(200, "text/html", getHtml(content, "Timer loopt"));
}

// Door de timer te stoppen wordt het verschil in ms berekend
// vanaf het moment van starten to het stopzetten. Daarna wordt
// die waarde opgeslagen en gebruikt als timeUntilFull
void handleStopTimer() {
    timeUntilFull = millis() - tempTime;
    digitalWrite(pinOpening, LOW);
    fireOff();
    redirect("/settings");
}

// Functie die de '/off' route afhandelt 
void handleOff() {
    // Draai de gaskraan dicht.
    fireOff();
    redirect("/");
}

// Functie die een 404 pagina niet gevonden fout afhandelt
// Dit is een functie uit het standaard voorbeeld van Webserver.h
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    // Loop door alle parameters (of geposte formdata inputs) heen
    // en zet de naam en waarde op het scherm
    for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    // Uit veiligheid draaien we de gaskraan dicht.
    fireOff();
    server.send(404, "text/plain", message);
}

// Functie die de afhandeling doet van '/fire?p=[10 - 100]'
void handleFire() {
    // p even omzetten naar int.
    int percentage = server.arg("p").toInt();
    // Gaskraan open zetten tot 'percentage'%
    fireOn(percentage);
    redirect("/");
}

// Functie die een nette 302 redirect doet naar een andere url.
// Zo kunnen we deze gebruiken om een handeling uit te voeren en
// daarna terug-/door te sturen naar een andere pagina (zie handleOff)
void redirect(String to) {
    // Zet de location header naar de URL
    server.sendHeader("Location", to);
    // Stuur je browser door met een 302. Niet een 301, want die worden
    // door de browser gecachet. Als je dan een foutje maakt, kun je lang
    // zoeken.
    server.send(302, "text/html");
}

// Functie die de gaskraan voor een bepaalde tijd openzet
// a.h.v percentage. Ik zal proberen in de functie m'n gedachtegang
// uit te leggen.
void fireOn(int percentage) {
    Serial.println("Haard naar " + String(percentage) + "%");

    // Mijn gashaard heeft dezelfde aansturing als een motortje van een
    // bestuurbare auto. De + en - zijn omkeerbaar en hoe om je ze hebt
    // aangesloten op de stroom bepaalt welke kant het motortje opdraait.
    //
    // Op die manier kun je dus de haard aanzetten of uitzetten. Ik had
    // bedacht dat als we de stroom stabiel kunnen houden (dus geen batterij,
    // maar een adapter) dan kunnen we er vanuit gaan dat het volledig open
    // draaien van het gas altijd even lang duurt vanaf het 0-punt.
    //
    // Zo kwam ik op timeUntilFull, wat een achterlijke naam is, maar
    // dit is de tijd die het duurt voordat de gashaard vanaf het 0-punt
    // tot maximaal aan staat. In de praktijk (met het 7.5V voltage dat ik
    // uiteindelijk gebruikte) bleek dit 6 seconden (ofwel 6000 ms) te zijn.
    // Conclusie: 6000 ms komt overeen met 100%.
    //
    // Nu is het bij mijn gashaard zo dat de eerste x procent van het draaien
    // er niets gebeurt. Althans, niet genoeg om de gashaard tot ontbranding
    // te laten komen. Die tijd dat het duurt voordat er wat gebeurt, heb ik
    // deadzone genoemd. Vrij logische naamkeuze toch? In eerste instantie
    // had ik hier een percentage van gemaakt, maar dit bleek in de praktijk
    // lastig te gebruiken als je zelf de max kon instellen via de timer.
    // Dus heb ik hier uiteindelijk seconden van gemaakt. En bleek de deadzone
    // zo'n 2 seconden.
    //
    // Dus hoe bereken je nou hoe lang je het motortje aan moet zetten?
    // Vanaf het 0-punt is het altijd 2000 ms voordat er altijd wat gebeurt.
    // Dus eigenlijk is 0% al 2000 ms. Dat betekent je dus altijd pas begint
    // vanaf die 2000. Stel dat ik hem op 40% zou willen zetten dan krijg je dus:
    // 6000 - 2000 = 4000 ms => dit is de tijd vanaf 2000 ms (0%) tot 6000 ms (100%).
    // DAAR neem je dus dan 40% van. Dus: 4000 * 0.4 = 1600 ms.
    // De totale tijd die de motor aan moet staan is: 2000 + 1600 = 3600 ms.
    //
    // Super slim, maar dit werkt natuurlijk alleen vanaf het 0-punt. Want
    // als we al op 40% zitten en we willen bijvoorbeeld naar 50% dan hoeven
    // we die eerste 2000 ms (de deadzone) niet voorbij. We willen dan eigenlijk
    // slechts die 10% extra erbij. Je krijgt dan het volgende:
    // ( 6000 - 2000 ) * ( 0.5 - 0.4 ) = 400 ms.
    // Dus de tijd na de deadzone tot volledig aan, maar dan 10% daarvan, om van
    // 40% naar 50% te komen. Andersom (dus van 40% naar 30% werkt hetzelfde,
    // alleen draai je dan die 400 ms de andere kant op).
    //
    // Dan is er alleen nog 1 ding. Stel dat het percentage 0 is, dan gebruiken we
    // de functie die speciaal is voor het uitzetten van het gas. Deze draait de
    // kraan nog net even wat langer door dan nodig, om te zorgen dat deze altijd
    // goed dicht gaat.

    if (percentage > 100)
    {
        // Hoewel het eigenlijk niet kan, kun je natuurlijk handmatig het percentage
        // groter maken dan 100% via de URL. Dat willen we natuurlijk voorkomen.
        percentage = 100;
    } else if (percentage <= 0)
    {
        // Hetzelfde geldt voor het uitzetten. Hoewel dit iets minder kwaad kan dan
        // de gaskraan te ver open draaien, is het wel vervelend als het systeem
        // gaat rekenen met -100%

        if (pinOpeningState == LOW && pinClosingState == LOW)
        {
            // De uitfunctie moet alleen gaan draaien als we niet al bezig zijn
            // met het uitzetten of aanzetten van de gaskraan.
            fireOff();
        }
        
    }
  
    // Werkt alleen als hij niet al bezig is met openen of sluiten
    // zodat de timing niet in de war raakt
    if (percentage > 0 && pinOpeningState == LOW && pinClosingState == LOW)
    {
        float timeInMs = 0;
    
        // Als het gewenste percentage hoger is dan het huidige percentage
        if (percentage > currentPercentage)
        {
            isOpening = true;
            isClosing = false;
            timeInMs = (timeUntilFull - timeDeadzone) * (percentage - currentPercentage) / 100;
        } else
        {
            isOpening = false;
            isClosing = true;
            timeInMs = (timeUntilFull - timeDeadzone) * (currentPercentage - percentage) / 100;
        }

        // Sla de nieuwe gegevens op
        currentPercentage = percentage;
        // Rond de berekening af (of nou ja, kap hem af) en sla de duur van hoe lang
        // de gaskraan open of dichtgedraaid moet worden op.
        timeToMove = (int) timeInMs;
        // Sla de tijd NU op, zodat we in de loop functie kunnen rekenen vanaf
        // deze tijd. Op die manier hoeven we geen delay te gebruiken.
        // In de loop() functie leg ik hier meer over uit.
        timeToMoveStart = millis();
    }
}

void fireOff() {
    // Dit werkt dus hetzelfde als hierboven uitgelegd met percentage 0
    // het grootste verschil is dat ik nog een extra 500 ms 'dichtdraai'. Dit heb
    // ik zo gedaan omdat ik bang was dat door afronding er telkens een paar ms
    // afwijking zou ontstaan. Waardoor je uiteindelijk (mogelijk) niet meer echt terug
    // naar het 0-punt komt. Door de extra 500 ms zal dit nooit het geval zijn en zal
    // hij altijd helemaal dicht draaien.
    //
    // Dat is dus ook de reden dat bij een percentage van 0, deze functie wordt aangeroepen
    // in plaats van fireOn(0);
    Serial.println("Haard gaat uit");
    float timeInMs = 500 + timeDeadzone + (timeUntilFull - timeDeadzone) * currentPercentage / 100;
    currentPercentage = 0;
    timeToMove = (int) timeInMs;
    timeToMoveStart = millis();
    isOpening = false;
    isClosing = true;
}

// MQTT dingen

// Functie die aangeroepen wordt zodra MQTT geconnect is met Adafruit.
    void onMqttConnect(bool sessionPresent) {
    Serial.printf("Connected to MQTT session=%d max payload FIXED\n",sessionPresent);
    Serial.println("Subscribing at QoS 2");
    // We 'abonneren' nu op het endpoint zodat we daar de meldingen van afvangen
    mqttClient.subscribe(aioEndPoint, 2);
    // Ik zet de ingebouwde LED van de arduino aan, zodat we kunnen zien dat we verbinding
    // hebben.
    digitalWrite(LED_BUILTIN, HIGH);
}

// Functie die wordt aangeroepen zodra MQTT de verbinding verbreekt.
void onMqttDisconnect(int8_t reason) {
    Serial.printf("Disconnected from MQTT reason=%d\n",reason);
    // Ik zet de LED uit, zodat we weten dat we geen verbinding meer hebben met Adafruit en dat
    // Google Assistent dus niet gaat werken
    digitalWrite(LED_BUILTIN, LOW);
}

// Functie die wordt aangeroepen zodra via MQTT een bericht binnenkomt
void onMqttMessage(const char* topic, uint8_t* payload, struct PANGO_PROPS props, size_t len, size_t index, size_t total) {
    // In IFTTT hebben we ingesteld dat we google een variabele laten meesturen als we zeggen:
    // Hey Google, set fireplace to {variabele}%.
    // Via Adafruit krijgen we die variabele hier binnen.

    // Hier wordt de payload omgezet naar een string array en vervolgens naar een int.
    // Geen idee of dit de meest efficiente manier is om te doen, maar ik kwam er anders
    // niet echt helemaal uit.
    int percentage = atoi((const char *)payload);
    Serial.println("GA: " + String(percentage) + "%");
    if (percentage == 0)
    {
        fireOff();
    } else if (percentage > 0)
    {
        fireOn(percentage);
    }
}

// Setup
void setup(void) {
    // We stellen de serial poort in op 115200 baud
    Serial.begin(115200);

    // We zetten alle pins die we gebruiken op OUTPUT, want we gaan ze niet uitlezen
    // alleen maar aansturen.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(pinOpening, OUTPUT);
    pinMode(pinClosing, OUTPUT);
  
    // We stellen Wifi in
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Debug info
    Serial.println("");

    // Wachten op de wifi verbinding
    while (WiFi.status() != WL_CONNECTED) {
        // JA ZEGT IE NET NOG DAT IE GEEN DELAY GEBRUIKT WAT IS DIT DAN ?!
        // Klopt, maar hier gebruik ik het ook om de rest van het script tegen te houden
        // dit hele apparaat is niets waard als er geen netwerkverbinding is. Het hoeft
        // niet eens een internetverbinding te zijn, als je hem maar kunt bereiken via
        // het netwerk.
        // Misschien dat we uiteindelijk ook knoppen erop kunnen zetten zodat je ook
        // handmatig misschien kunt instellen, maar dan wil ik ook dat je het systeem
        // visueel aangeeft hoeveel procent de haard aan staat, met LEDs bijvoorbeeld.
        delay(500);
        Serial.print(".");
    }

    // Debug info
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // We maken gebruik van MDNS zodat op alle devices behalve Android je de webserver
    // kunt benaderen via 'domeinnaam'.local (zonder quotes natuurlijk). Zo hoef je dus
    // niet het IP-adres te weten van de Arduino (ESP32).
    if (MDNS.begin(domain)) {
        Serial.println("MDNS responder started");
    }

    // Hier stellen we in welke URLs op de webserver welke functies aanroepen.
    server.on("/", handleRoot);
    server.on("/off", handleOff);
    server.on("/fire", handleFire);
    server.on("/settings", handleSettings);
    server.on("/settings/deadzone", handleDeadzone);
    server.on("/settings/startTimer", handleStartTimer);
    server.on("/settings/stopTimer", handleStopTimer);
    server.onNotFound(handleNotFound);

    // Hier stellen we in welke functies worden aangeroepen vanuit MQTT als er een event
    // plaatsvindt.
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);

    // Stel MQTT in op Adafruit met de gegevens die je bovenin hebt gedefinieerd
    mqttClient.setServer(aioServer, aioPort);
    mqttClient.setCredentials(aioUsername, aioKey);
    // Verbindt MQTT
    mqttClient.connect();

    // Start de webserver
    server.begin();
    Serial.println("HTTP server started");
}

void loop(void) {
    // Dit is de standaard functie van de webserver die waarschijnlijk elke loop
    // de afhandeling van requests doet
    server.handleClient();

    // Vanuit de fireOn en fireOff functies wordt bepaald of we op het moment de
    // gaskraan aan het openen of sluiten zijn. Op basis van wat we aan het doen zijn
    // komt de stroom op de ene of de andere pin te staan.
    if (isOpening) {
        // DeltaTime gebruiken we om te bepalen hoe lang we al met deze actie bezig zijn.
        // vanuit de fireOn en fireOff functies wordt 'timeToMoveStart' ingesteld.
        // Hiermee bepalen we als het ware het 0-punt. Stel dat timeToMoveStart = 10000 ms
        // en we komen in deze functie en millis() = 12000, betekent dit dat we:
        // 12000 - 10000 = 2000 ms bezig zijn. Op die manier kun je dus berekenen hoe
        // lang je bezig bent, en zorgen dat hij pas iets gaat doen zodra hij een bepaald
        // punt voorbij is. Als die berekening van 'deltaTime' in dit geval groter is dan
        // 'timeToMove'. timeToMove wordt ook gezet in de fireOn en fireOff functies. Dit
        // is namelijk de tijd dat de gaskraan moet worden opengezet of gesloten. Die hele
        // berekening waar ik het in die functie over heb.
        int deltaTime = millis() - timeToMoveStart;

        // Zodra deltaTime groter of gelijk aan de tijd is die we in fireOn of fireOff
        // hebben berekend, dan
        if (deltaTime >= timeToMove)
        {
            // Stoppen we met openen
            isOpening = false;
            // Halen we de stroom van de open PIN af zodat de motor stopt met draaien
            digitalWrite(pinOpening, LOW);
            // Slaan we die state op zodat de loop weet dat we niets meer aan het doen zijn
            pinOpeningState = LOW;
      
            // Bij het starten hebben we de debugTimer gezet. Op deze manier kunnen we zien
            // of het openen van de gaskraan inderdaad zo lang aan heeft gestaan als dat we 
            // hadden gewild. Dit zit er door afronding meestal 1 ms naast.
            Serial.println("... done in: " + String(millis() - debugTimer) + " ms, expected: " + String(timeToMove) + " ms");
        } else
        {
            // Als deltaTime niet groter is dan timeToMove dan betekent dit dat we bezig
            // zouden moeten zijn! Maar natuurlijk alleen als we niet al bezig zijn.
            // Dat checken we a.h.v. pinOpeningState. Dus als die op LOW staat, betekent dit
            // dat we niets aan het doen zijn. De motor staat uit, maar omdat we wel in deze
            // if zijn beland (isOpening == true) betekent dit dat we dus aan zouden moeten gaan.
            if (pinOpeningState == LOW)
            {
                // We moeten de motor aanzetten en zetten we dus stroom op de pinOpening PIN.
                // Dit zorgt ervoor dat de DC Motorcontroller de motor gaan aansturen
                digitalWrite(pinOpening, HIGH);
                // Die state slaan we op, zodat de loop straks weet dat we al bezig zijn en
                // dat er dus niets hoeft te gebeuren.
                pinOpeningState = HIGH;
                // Voor de debug slaan we de huidige tijd (het aantal milli seconden vanaf de
                // start van het script) op.
                debugTimer = millis();
                Serial.println("Turning ON fireplace...");
            }
            // Else: Doe niets, want we moeten nog niet stoppen met open gaan
        }
    } else if (isClosing) {
        // Dit werkt precies hetzelfde als hierbovenstaande, alleen dus voor het sluiten van
        // de gaskraan.
        int deltaTime = millis() - timeToMoveStart;
        if (deltaTime >= timeToMove)
        {
            isClosing = false;
            digitalWrite(pinClosing, LOW);
            pinClosingState = LOW;

            Serial.println("... done in: " + String(millis() - debugTimer) + " ms, expected: " + String(timeToMove) + " ms");
        } else
        {
            if (pinClosingState == LOW)
            {
                digitalWrite(pinClosing, HIGH);
                pinClosingState = HIGH;

                debugTimer = millis();
                Serial.println("Turning OFF fireplace...");
            }
        }
    }
}
