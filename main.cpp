#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"

using namespace transport_catalogue;
using namespace std::literals;

int main() {
    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
     * с ответами.
     * Вывести в stdout ответы в виде JSON
     */

    TransportCatalogue tc;
    JsonReader reader(tc);
    reader.ReadJson_FillTransportCatalogue(std::cin);
    reader.QueryTC_WriteJsonToStream(std::cout);
}