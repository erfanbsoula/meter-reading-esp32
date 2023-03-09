class RectangleMoveContext {
    constructor(recangle) {
        this.recangle = recangle;
        this.canvas = recangle.parentElement;
        this.move = false;
        this.movement = {
            x1: 0,
            y1: 0,
            x2: 0,
            y2: 0
        };
    }

    isInCanvas(dx, dy) {
        let top = 0, left = 0;
        let bottom = this.canvas.offsetHeight;
        let right = this.canvas.offsetWidth;

        let rect_top = this.recangle.offsetTop + dy;
        let rect_bottom = rect_top + this.recangle.offsetHeight;
        let rect_left = this.recangle.offsetLeft + dx;
        let rect_right = rect_left + this.recangle.offsetWidth;

        return [left <= rect_left && rect_right <= right,
            top <= rect_top && rect_bottom <= bottom];
    }

    mouseDownHandler(event) {
        this.move = true;
        this.movement.x1 = event.pageX;
        this.movement.y1 = event.pageY;
    }

    mouseUpHandler(event) {
        this.mouseMoveHandler(event);
        this.move = false;
    }

    mouseMoveHandler(event) {
        if (this.move) {

            this.movement.x2 = event.pageX;
            this.movement.y2 = event.pageY;

            let dx = this.movement.x2 - this.movement.x1;
            let dy = this.movement.y2 - this.movement.y1;
            let validMove = this.isInCanvas(dx, dy);

            if (validMove[0]) {
                this.recangle.style.left = this.recangle.offsetLeft + dx + 'px';
                this.movement.x1 = this.movement.x2;
            }
            if (validMove[1]) {
                this.recangle.style.top = this.recangle.offsetTop + dy + 'px';
                this.movement.y1 = this.movement.y2;
            }
        }
    }
}

class RectangleDrawerContext {

    constructor(canvas) {
        this.canvas = canvas;
        this.maxRectCount = 0;
        this.maxRectSize = 0;
        this.rectangleMoveContexts = [];
        this.rectangles = [];
        this.count = 0;
        this.draw = false;
        this.element = null;
        this.mouse_in = true;
        this.mouse = {
            x1: 0,
            y1: 0,
            x2: 0,
            y2: 0
        };
    }

    setConfig(maxRectCount, maxRectSize) {
        this.maxRectCount = maxRectCount;
        this.maxRectSize = maxRectSize;
    }

    isInCanvas(x, y) {
        let top = this.canvas.offsetTop;
        let bottom = top + this.canvas.offsetHeight;
        let left = this.canvas.offsetLeft;
        let right = left + this.canvas.offsetWidth;
        return top <= y && y <= bottom && left <= x && x <= right;
    }

    makeMousePositionValid() {
        if (Math.abs(this.mouse.x1 - this.mouse.x2) > this.maxRectSize) {
            if (this.mouse.x1 < this.mouse.x2)
                this.mouse.x1 = this.mouse.x2 - this.maxRectSize;
            else
                this.mouse.x1 = this.mouse.x2 + this.maxRectSize;
        }
        if (Math.abs(this.mouse.y1 - this.mouse.y2) > this.maxRectSize) {
            if (this.mouse.y1 < this.mouse.y2)
                this.mouse.y1 = this.mouse.y2 - this.maxRectSize;
            else
                this.mouse.y1 = this.mouse.y2 + this.maxRectSize;
        }
    }

    setMousePosition(event) {
        let x = event.pageX;
        let y = event.pageY;
        if (!this.isInCanvas(x, y)) {
            return;
        }

        this.mouse.x1 = x;
        this.mouse.y1 = y;
    }

    updateElementShape() {
        if (this.element !== null) {
            let top = this.canvas.offsetTop;
            let left = this.canvas.offsetLeft;
            this.makeMousePositionValid();
            this.element.style.width = Math.abs(this.mouse.x1 - this.mouse.x2) + 'px';
            this.element.style.height = Math.abs(this.mouse.y1 - this.mouse.y2) + 'px';
            this.element.style.left = (this.mouse.x1 < this.mouse.x2) ? this.mouse.x1-left + 'px' : this.mouse.x2-left + 'px';
            this.element.style.top = (this.mouse.y1 < this.mouse.y2) ? this.mouse.y1-top + 'px' : this.mouse.y2-top + 'px';
        }
    }

    enableDrawing() { this.draw = true; }
    disableDrawing() { this.draw = false; }

    createRectangle() {
        this.element = document.createElement('div');
        this.element.className = 'rectangle';
        this.element.style.left = this.mouse.x1 - this.canvas.offsetLeft + 'px';
        this.element.style.top = this.mouse.y1 - this.canvas.offsetTop + 'px';
        this.canvas.appendChild(this.element);
    }

    mouseMoveHandler(event) {
        this.setMousePosition(event);
        this.updateElementShape();
    }

    mouseClickHandler() {
        if (this.element !== null) {
            this.canvas.style.cursor = "default";
            let rectangleMoveContext = new RectangleMoveContext(this.element);
            this.element.addEventListener('mousedown', (event) => {
                this.disableDrawing();
                rectangleMoveContext.mouseDownHandler(event);
            });
            this.element.addEventListener('mouseup', (event) => { rectangleMoveContext.mouseUpHandler(event) });
            this.element.addEventListener('mousemove', (event) => { rectangleMoveContext.mouseMoveHandler(event) });
            this.element.addEventListener('mouseleave', (event) => { drawer.enableDrawing() });
            this.rectangleMoveContexts.push(rectangleMoveContext);
            this.rectangles.push(this.element);
            this.element = null;
            this.count = this.count + 1;
        }
        else if (this.count < this.maxRectCount && this.draw) {
            this.mouse.x2 = this.mouse.x1;
            this.mouse.y2 = this.mouse.y1;
            this.createRectangle();
            this.canvas.style.cursor = "crosshair";
        }
    }

    deleteAllRectangles() {
        while (0 < this.rectangles.length) {
            this.deleteLastRectangle();
        }
        this.count = 0;
    }

    deleteLastRectangle() {
        if (this.count > 0) {
            this.rectangles.pop().remove();
            this.rectangleMoveContexts.pop();
            this.count -= 1;
        }
    }
}

var drawer = new RectangleDrawerContext(document.getElementById('camera-canvas'));
drawer.canvas.addEventListener('mousemove', (event) => { drawer.mouseMoveHandler(event); });
drawer.canvas.addEventListener('click', (event) => { drawer.mouseClickHandler(); });
window.addEventListener('resize', (event) => { drawer.deleteAllRectangles() });
// drawer.enableDrawing();
