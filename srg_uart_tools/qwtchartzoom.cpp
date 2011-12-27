/**************************************************************/
/*                                                            */
/*               Реализация класса QwtChartZoom               */
/*                         Версия 1.1                         */
/*                                                            */
/* Разработал Мельников Сергей Андреевич,                     */
/* г. Каменск-Уральский Свердловской обл., 2011 г.            */
/*                                                            */
/* Разрешается свободное использование и распространение.     */
/* Упоминание автора обязательно.                             */
/*                                                            */
/**************************************************************/

#include "qwtchartzoom.h"

// Конструктор
QwtChartZoom::QwtChartZoom(QwtPlot *qp,QObject *parent) :
    QObject(parent)
{
    // сбрасываем флаг для того, чтобы перед первым изменением масштаба
    // текущие границы графика были зафиксированы в качестве исходных
    isbF = false;
    // назначаем коэффициент, определяющий изменение масштаба графика
    // при вращении колеса мыши
    sfact = 1.2;
    // получаем компонент QwtPlot, над которым будут производиться все преобразования
    qwtp = qp;
    // устанавливаем ему свойство, разрешающее обрабатывать события от клавиатуры
    qwtp->setFocusPolicy(Qt::StrongFocus);
    // очищаем виджет, отвечающий за отображение выделенной области
    zwid = 0;
    // и назначаем ему цвет (по умолчанию - черный)
    zwClr = Qt::black;
    // сбрасываем признак режима
    convType = ctNone;
    // устанавливаем обработчик всех событий
    qwtp->installEventFilter(this);
}

// Установка цвета рамки, задающей новый размер графика
void QwtChartZoom::setRubberBandColor(QColor clr) {
    zwClr = clr;
}

// Установка курсора
void QwtChartZoom::setCursor(QCursor cur) {
    qwtp->canvas()->setCursor(cur);
}

// Фиксация текущих границ графика в качестве исходных
void QwtChartZoom::fixBoundaries() {
    // здесь только сбрасывается флаг и тем самым
    // указывается на необходимость фиксировать границы
    isbF = false;
    // фактическая фиксация границ произойдет в момент начала
    // какого-либо преобразования при вызове fixBounds()
}

// Задание коэффициента масштабирования графика
// при вращении колеса мыши (по умолчанию он равен 1.2)
void QwtChartZoom::setWheelFactor(double fact) {
    sfact = fact;
}

// Сохранение текущего курсора
void QwtChartZoom::storeCursor() {
    tCursor = qwtp->canvas()->cursor();
}

// Восстановление курсора
void QwtChartZoom::restCursor() {
    qwtp->canvas()->setCursor(tCursor);
}

// Обработчик всех событий
bool QwtChartZoom::eventFilter(QObject *obj,QEvent *event)
{
    // если событие имеет отношение к QwtPlot
    if (obj == qwtp)
    {
        // если произошло одно из событий от мыши, то вызываем обработчик procMouseEvent
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) procMouseEvent(event);

        // если произошло одно из событий от клавиатуры, то вызываем обработчик switchWheel
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease) switchWheel(event);

        // если событие вызвано вращением колеса мыши, то вызываем обработчик procWheel
        if (event->type() == QEvent::Wheel) procWheel(event);
    }
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(obj,event);
}

// Обработчик обычных событий от мыши
void QwtChartZoom::procMouseEvent(QEvent *event)
{
    // создаем указатель на событие от мыши
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    // если нажата кнопка мыши, то вызываем обработчик procMouseButtonPress
    if (event->type() == QEvent::MouseButtonPress) procMouseButtonPress(mEvent);
    // иначе если мышь, то вызываем обработчик procMouseMove
    else if (event->type() == QEvent::MouseMove) procMouseMove(mEvent);
    // иначе если отпущена кнопка мыши, то вызываем обработчик procMouseButtonRelease
    else if (event->type() == QEvent::MouseButtonRelease) procMouseButtonRelease(mEvent);
}

// Обработчик нажатия на кнопку мыши
void QwtChartZoom::procMouseButtonPress(QMouseEvent *mEvent)
{
    // фиксируем исходные границы графика (если этого еще не было сделано)
    fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (convType == ctNone)
    {
        // определяем текущее положение курсора (относительно канвы QwtPlot)
        scp_x = mEvent->pos().x() - qwtp->canvas()->geometry().x();
        scp_y = mEvent->pos().y() - qwtp->canvas()->geometry().y();
        // если курсор находится над канвой QwtPlot
        if (scp_x >= 0 &&
            scp_x < qwtp->canvas()->geometry().width() &&
            scp_y >= 0 &&
            scp_y < qwtp->canvas()->geometry().height())
        {
            // если нажата левая кнопка мыши, то начинаем выделять область
            // для перемасштабирования графика
            if (mEvent->button() == Qt::LeftButton) startZoom();
            // иначе если нажата правая кнопка мыши, то включаем режим
            // перемещения графика
            else if (mEvent->button() == Qt::RightButton) startDrag();
        }
    }
}

// Обработчик перемещения мыши
void QwtChartZoom::procMouseMove(QMouseEvent *mEvent)
{
    // если включен режим изменения масштаба, то
    // выполняем выделение области для перемасштабирования графика
    if (convType == ctZoom) selectZoomRect(mEvent);
    // иначе если включен режим перемещения графика, то
    // выполняем перемещение графика
    else if (convType == ctDrag) execDrag(mEvent);
}

// Обработчик отпускания кнопки мыши
void QwtChartZoom::procMouseButtonRelease(QMouseEvent *mEvent)
{
    // если включен режим изменения масштаба или режим перемещения графика
    if (convType==ctZoom || convType==ctDrag)
    {
        // если отпущена левая кнопка мыши, то
        // выполняем перемасштабирование графика в соответствии с выделенной областью
        if (mEvent->button() == Qt::LeftButton) execZoom(mEvent);
        // иначе если отпущена правая кнопка мыши, то
        // прекращаем перемещение графика
        else if (mEvent->button() == Qt::RightButton) endDrag();
    }
}

// Включение изменения масштаба
void QwtChartZoom::startZoom()
{
    // прописываем соответствующий признак режима
    convType = ctZoom;
    storeCursor();  // запоминаем текущий курсор
    // устанавливаем курсор Cross
    setCursor(Qt::CrossCursor);
    // создаем виджет, который будет отображать выделенную область
    // (он будет прорисовываться на том же виджете, что и QwtPlot)
    zwid = new QWidget(qwtp->parentWidget());
    // и назначаем ему цвет
    zwid->setStyleSheet(QString(
        "background-color:rgb(%1,%2,%3);").arg(
        zwClr.red()).arg(zwClr.green()).arg(zwClr.blue()));
}

// Выделение новых границ графика для изменения масштаба
void QwtChartZoom::selectZoomRect(QMouseEvent *mEvent)
{
    // scp_x - координата курсора в пикселах по оси x в начальный момент времени
    //     (когда была нажата левая кнопка мыши)
    // mEvent->pos().x() - qwtp->canvas()->geometry().x() - координата курсора
    //     в пикселах по оси x в текущий момент времени
    // mEvent->pos().x() - qwtp->canvas()->geometry().x() - scp_x - смещение курсора
    //     в пикселах по оси x от начального положения и соответственно
    //     ширина dx выделенной области
    int dx = mEvent->pos().x() - qwtp->canvas()->geometry().x() - scp_x;
    // qwtp->geometry().x() - положение QwtPlot по оси x относительно виджета, его содержащего
    // qwtp->geometry().x() + qwtp->canvas()->geometry().x() - положение канвы QwtPlot
    //     по оси x относительно виджета, его содержащего
    // qwtp->geometry().x() + qwtp->canvas()->geometry().x() + scp_x - положение gx0 начальной точки
    //     по оси x относительно виджета, содержащего QwtPlot, она нужна в качестве опоры
    //     для отображения выделенной области
    int gx0 = qwtp->geometry().x() + qwtp->canvas()->geometry().x() + scp_x;
    // если ширина выделенной области отрицательна, то текущая точка находится левее начальной,
    //     и тогда именно ее мы используем в качестве опоры для отображения выделенной области
    if (dx < 0) {dx = -dx; gx0 -= dx;}
    // иначе если ширина равна нулю, то для того чтобы выделенная область все-таки отбражалась,
    //     принудительно сделаем ее равной единице
    else if (dx == 0) dx = 1;
    // аналогично определяем высоту dy выделенной области
    int dy = mEvent->pos().y() - qwtp->canvas()->geometry().y() - scp_y;
    // и положение gy0 начальной точки по оси y
    int gy0 = qwtp->geometry().y() + qwtp->canvas()->geometry().y() + scp_y;
    // если высота выделенной области отрицательна, то текущая точка находится выше начальной,
    //     и тогда именно ее мы используем в качестве опоры для отображения выделенной области
    if (dy < 0) {dy = -dy; gy0 -= dy;}
    // иначе если высота равна нулю, то для того чтобы выделенная область все-таки отбражалась,
    //     принудительно сделаем ее равной единице
    else if (dy == 0) dy = 1;
    // устанавливаем положение и размеры виджета, отображающего выделенную область
    zwid->setGeometry(gx0,gy0,dx,dy);
    // формируем маску для виджета, отображающего выделенную область
    // непрозрачная область
    QRegion rw(0,0,dx,dy);
    // прозрачная область
    QRegion rs(1,1,dx-2,dy-2);
    // устанавливаем маску путем вычитания из непрозрачной области прозрачной
    zwid->setMask(rw.subtracted(rs));
    // делаем виджет, отображающего выделенную область, видимым
    zwid->setVisible(true);
    // перерисовываем QwtPlot
    zwid->repaint();
}

// Выполнение изменения масштаба
void QwtChartZoom::execZoom(QMouseEvent *mEvent)
{
    // если включен режим изменения масштаба
    if (convType == ctZoom)
    {
        restCursor();   // восстанавливаем курсор
        // удаляем виджет, отображающий выделенную область
        delete zwid;
        // определяем положение курсора, т.е. координаты xp и yp
        // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
        int xp = mEvent->pos().x() - qwtp->canvas()->geometry().x();
        int yp = mEvent->pos().y() - qwtp->canvas()->geometry().y();
        // если выделение производилось справа налево или снизу вверх,
        // то восстанавливаем исходные границы графика (отменяем увеличение)
        if (xp<scp_x || yp<scp_y) restBounds();
        // иначе если размер выделенной области достаточен, то изменяем масштаб
        else if (xp-scp_x >= 8 && yp-scp_y >= 8)
        {
            // устанавливаем левую границу шкалы x по начальной точке, а правую по конечной
            qwtp->setAxisScale(QwtPlot::xBottom,
                               qwtp->invTransform(QwtPlot::xBottom,scp_x),
                               qwtp->invTransform(QwtPlot::xBottom,xp));
            // устанавливаем верхнюю границу шкалы y по начальной точке, а нижнюю по конечной
            qwtp->setAxisScale(QwtPlot::yLeft,
                               qwtp->invTransform(QwtPlot::yLeft,yp),
                               qwtp->invTransform(QwtPlot::yLeft,scp_y));
            // перерисовываем график
            qwtp->replot();
        }
        // очищаем признак режима
        convType = ctNone;
    }
}

// Включение перемещения графика
void QwtChartZoom::startDrag()
{
    // прописываем соответствующий признак режима
    convType = ctDrag;
    storeCursor();  // запоминаем текущий курсор
    // устанавливаем курсор OpenHand
    setCursor(Qt::OpenHandCursor);
    // определяем текущий масштабирующий множитель по оси x
    //     (т.е. узнаем на сколько изменяется координата по шкале x
    //     при перемещении курсора вправо на один пиксел)
    cs_kx = qwtp->invTransform(QwtPlot::xBottom,scp_x+1) - qwtp->invTransform(QwtPlot::xBottom,scp_x);
    // аналогично определяем текущий масштабирующий множитель по оси y
    cs_ky = qwtp->invTransform(QwtPlot::yLeft,scp_y+1) - qwtp->invTransform(QwtPlot::yLeft,scp_y);
    // получаем карту шкалы x на канве
    QwtScaleMap sm = qwtp->canvasMap(QwtPlot::xBottom);
    // для того чтобы фиксировать начальные левую и правую границы
    scb_xl = sm.s1(); scb_xr = sm.s2();
    // аналогично получаем карту шкалы y на канве
    sm = qwtp->canvasMap(QwtPlot::yLeft);
    // для того чтобы фиксировать начальные нижнюю и верхнюю границы
    scb_yb = sm.s1(); scb_yt = sm.s2();
}

// Выполнение перемещения графика
void QwtChartZoom::execDrag(QMouseEvent *mEvent)
{
    // устанавливаем курсор ClosedHand
    setCursor(Qt::ClosedHandCursor);
    // scp_x - координата курсора в пикселах по оси x в начальный момент времени
    //     (когда была нажата правая кнопка мыши)
    // mEvent->pos().x() - qwtp->canvas()->geometry().x() - координата курсора
    //     в пикселах по оси x в текущий момент времени
    // mEvent->pos().x() - qwtp->canvas()->geometry().x() - scp_x - смещение курсора
    //     в пикселах по оси x от начального положения
    // (mEvent->pos().x() - qwtp->canvas()->geometry().x() - scp_x) * cs_kx -  это же смещение,
    //     но уже в единицах шкалы x
    // dx - смещение границ по оси x берется с обратным знаком
    //     (чтобы график относительно границ переместился вправо, сами границы следует сместить влево)
    double dx = -(mEvent->pos().x() - qwtp->canvas()->geometry().x() - scp_x) * cs_kx;
    // устанавливаем новые левую и правую границы шкалы для оси x
    //     новые границы = начальные границы + смещение
    qwtp->setAxisScale(QwtPlot::xBottom,scb_xl+dx,scb_xr+dx);
    // аналогично определяем dy - смещение границ по оси y
    double dy = -(mEvent->pos().y() - qwtp->canvas()->geometry().y() - scp_y) * cs_ky;
    // устанавливаем новые нижнюю и верхнюю границы шкалы для оси y
    qwtp->setAxisScale(QwtPlot::yLeft,scb_yb+dy,scb_yt+dy);
    // перерисовываем график
    qwtp->replot();
}

// Выключение перемещения графика
void QwtChartZoom::endDrag()
{
    // если включен режим перемещения графика
    if (convType == ctDrag)
    {
        restCursor();       // то восстанавливаем курсор
        convType = ctNone;  // и очищаем признак режима
    }
}

// Обработчик нажатия/отпускания клавиши Ctrl
void QwtChartZoom::switchWheel(QEvent *event)
{
    // создаем указатель на событие от клавиатуры
    QKeyEvent *kEvent = static_cast<QKeyEvent *>(event);
    // если нажата/отпущена клавиша Ctrl
    if (kEvent->key() == Qt::Key_Control)
    {
        // если клавиша нажата
        if (event->type() == QEvent::KeyPress)
        {
            // если не включен никакой другой режим,
            // то включаем режим Wheel
            if (convType==ctNone) convType = ctWheel;
        }
        // иначе если клавиша отпущена
        else if (event->type() == QEvent::KeyRelease)
        {
            // если включен режим Wheel,
            // то выключаем его
            if (convType == ctWheel) convType = ctNone;
        }
    }
}

// Обработчик вращения колеса мыши
void QwtChartZoom::procWheel(QEvent *event)
{
    // если включен режим Wheel (была нажата клавиша Ctrl)
    if (convType == ctWheel)
    {
        // приводим тип QEvent к QWheelEvent
        QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);
        // если вращается вертикальное колесо мыши
        if (wEvent->orientation() == Qt::Vertical)
        {
            // фиксируем исходные границы графика (если этого еще не было сделано)
            fixBounds();
            // получаем карту шкалы оси x на канве
            QwtScaleMap sm = qwtp->canvasMap(QwtPlot::xBottom);
            // определяем центр отображаемого на шкале x интервала
            double mx = (sm.s1() + sm.s2()) / 2;
            // и полуширину интервала
            double dx = (sm.s2() - sm.s1()) / 2;
            // то же самое проделываем с осью y
            sm = qwtp->canvasMap(QwtPlot::yLeft);
            // определяем центр отображаемого на шкале y интервала
            double my = (sm.s1() + sm.s2()) / 2;
            // и полуширину интервала
            double dy = (sm.s2() - sm.s1()) / 2;
            // определяем угол поворота колеса мыши
            // (значение 120 соответствует углу поворота 15°)
            int wd = wEvent->delta();
            // вычисляем масштабирующий множитель
            // (во сколько раз будет увеличен/уменьшен график)
            double kw = sfact * wd / 120;
            // если угол поворота положителен, то изображение графика увеличивается
            if (wd > 0)
            {
                // уменьшаем полуширину отображаемых интервалов в kw раз
                dx /= kw; dy /= kw;
                // устанавливаем новые левую и правую границы шкалы для оси x
                // (центр изображаемой части графика остается на месте,
                // а границы приближаются к центру, т.о. изображение графика увеличивается)
                qwtp->setAxisScale(QwtPlot::xBottom,mx-dx,mx+dx);
                // аналогично устанавливаем новые нижнюю и верхнюю границы шкалы для оси y
                qwtp->setAxisScale(QwtPlot::yLeft,my-dy,my+dy);
                // перерисовываем график
                qwtp->replot();
            }
            // иначе если угол поворота отрицателен, то изображение графика уменьшается
            else if (wd < 0)
            {
                // увеличиваем полуширину отображаемых интервалов в -kw раз
                dx *= -kw; dy *= -kw;
                // устанавливаем новые левую и правую границы шкалы для оси x
                // (центр изображаемой части графика остается на месте,
                // а границы удаляются от центра, т.о. изображение графика уменьшается)
                qwtp->setAxisScale(QwtPlot::xBottom,mx-dx,mx+dx);
                // аналогично устанавливаем новые нижнюю и верхнюю границы шкалы для оси y
                qwtp->setAxisScale(QwtPlot::yLeft,my-dy,my+dy);
                // перерисовываем график
                qwtp->replot();
            }
        }
    }
}

// Фактическая фиксация текущих границ графика
// в качестве исходных (если флаг isbF сброшен)
void QwtChartZoom::fixBounds()
{
    // если этого еще не было сделано
    if (!isbF)
    {
        // получаем карту шкалы оси x на канве
        QwtScaleMap sm = qwtp->canvasMap(QwtPlot::xBottom);
        // и запоминаем текущие левую и правую границы шкалы
        isb_xl = sm.s1(); isb_xr = sm.s2();
        // то же самое проделываем с осью y
        sm = qwtp->canvasMap(QwtPlot::yLeft);
        // запоминаем текущие нижнюю и верхнюю границы шкалы
        isb_yb = sm.s1(); isb_yt = sm.s2();
        // устанавливаем флажок фиксации границ графика
        isbF = true;
    }

}

// Восстановление исходных границ графика
void QwtChartZoom::restBounds()
{
    // устанавливаем запомненные ранее
    // левую и правую границы шкалы для оси x
    qwtp->setAxisScale(QwtPlot::xBottom,isb_xl,isb_xr);
    // и нижнюю и верхнюю границы шкалы для оси y
    qwtp->setAxisScale(QwtPlot::yLeft,isb_yb,isb_yt);
    // перерисовываем график
    qwtp->replot();
}
