
#include "Precomp.h"
#include "Element.h"
#include "Event.h"
#include "WindowFrame.h"
#include "Canvas.h"
#include "ComputedBorder.h"

std::unique_ptr<Element> Element::createElement(std::string elementType)
{
	return std::make_unique<Element>();
}

std::unique_ptr<Element> Element::createElementNS(std::string ns, std::string elementType)
{
	return createElement(elementType);
}

Element::Element()
{
}

Element::~Element()
{
}

void Element::appendChild(Element* element)
{
	element->setParent(this);
	setNeedsLayout();
}

void Element::insertBefore(Element* newNode, Element* referenceNode)
{
	newNode->setParent(this);
	newNode->moveBefore(referenceNode);
	setNeedsLayout();
}

void Element::removeChild(Element* element)
{
	element->detachFromParent();
	setNeedsLayout();
}

void Element::setAttribute(std::string name, std::string value)
{
	if (name == "class")
	{
		classes.clear();
		size_t pos = 0;
		while (pos < value.size())
		{
			pos = value.find_first_not_of(" \t", pos);
			if (pos == std::string::npos)
				break;

			size_t pos2 = value.find_first_of(" \t", pos);
			if (pos2 == std::string::npos)
				pos2 = value.size();

			if (pos < pos2)
				classes.insert(value.substr(pos, pos2 - pos));

			pos = pos2;
		}
		setNeedsLayout();
	}
	else
	{
		attributes[name] = value;
	}
	setNeedsLayout();
}

void Element::removeAttribute(std::string name)
{
	if (name == "class")
	{
		classes.clear();
		setNeedsLayout();
	}
	else
	{
		auto it = attributes.find(name);
		if (it != attributes.end())
			attributes.erase(it);
	}
	setNeedsLayout();
}

void Element::setStyle(std::string name, std::string value)
{
	if (name == "width" && value.size() > 2 && value.substr(value.size() - 2) == "px")
	{
		fixedWidth = std::atof(value.substr(0, value.size() - 2).c_str());
	}
	else if (name == "height" && value.size() > 2 && value.substr(value.size() - 2) == "px")
	{
		fixedHeight = std::atof(value.substr(0, value.size() - 2).c_str());
	}
}

void Element::click()
{
	dispatchEvent("click");
}

void Element::focus()
{
}

void Element::dispatchEvent(std::string name, Event* e, bool bubbles)
{
	Element* currentTarget = this;
	while (true)
	{
		for (auto& handler : currentTarget->eventListeners[name])
		{
			handler(e);
			if (e->stopImmediatePropagationFlag)
				break;
		}

		currentTarget = currentTarget->parent();
		if (!bubbles || !currentTarget || e->stopPropagationFlag || e->stopImmediatePropagationFlag)
			break;
	}

	if (!e->preventDefaultFlag)
	{
		defaultAction(e);
	}
}

void Element::dispatchEvent(std::string name, bool bubbles)
{
	Event e;
	dispatchEvent(name, &e, bubbles);
}

void Element::addEventListener(std::string name, std::function<void(Event* event)> handler)
{
	eventListeners[name].push_back(std::move(handler));
}

Rect Element::getBoundingClientRect()
{
	Rect box = geometry().contentBox();
	Point tl = toRootPos(box.topLeft());
	Point br = toRootPos(box.bottomRight());
	return Rect::ltrb(tl.x, tl.y, br.x, br.y);
}

double Element::clientTop() const
{
	return 0;
}

double Element::clientLeft() const
{
	return 0;
}

double Element::clientWidth() const
{
	return 0;
}

double Element::clientHeight() const
{
	return 0;
}

double Element::offsetLeft() const
{
	return 0;
}

double Element::offsetTop() const
{
	return 0;
}

double Element::offsetWidth() const
{
	return 0;
}

double Element::offsetHeight() const
{
	return 0;
}

double Element::scrollLeft() const
{
	return 0;
}

double Element::scrollTop() const
{
	return 0;
}

double Element::scrollWidth() const
{
	return 0;
}

double Element::scrollHeight() const
{
	return 0;
}

void Element::scrollTo(double x, double y)
{
}

void Element::scrollBy(double x, double y)
{
}

void Element::setScrollLeft(double x)
{
}

void Element::setScrollTop(double y)
{
}

std::string Element::getValue()
{
	return {};
}

void Element::setInnerText(const std::string& text)
{
	innerText = text;
	setNeedsLayout();
}

void Element::setInnerHTML(const std::string& text)
{
}

std::string Element::getInnerText()
{
	return std::string();
}

std::string Element::getInnerHTML()
{
	return std::string();
}

Element* Element::findElementAt(const Point& pos)
{
	for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
	{
		if (element->geometry().borderBox().contains(pos))
		{
			return element->findElementAt(pos - element->geometry().contentPos());
		}
	}
	return this;
}

bool Element::needsLayout() const
{
	return needs_layout;
}

void Element::clearNeedsLayout()
{
	needs_layout = false;
}

void Element::setNeedsLayout()
{
	Element* super = parent();
	if (super)
		super->setNeedsLayout();
	else
		setNeedsRender();
}

const ElementGeometry& Element::geometry() const
{
	return elementgeometry;
}

void Element::setGeometry(const ElementGeometry& geometry)
{
	if (elementgeometry.contentBox() != geometry.contentBox())
	{
		elementgeometry = geometry;
		setNeedsLayout();
	}
}

void Element::setNeedsRender()
{
	WindowFrame* w = window();
	if (w)
		w->setNeedsRender();
}

Point Element::toRootPos(const Point& pos)
{
	if (parent())
		return parent()->toRootPos(geometry().contentBox().pos() + pos);
	else
		return pos;
}

Point Element::fromRootPos(const Point& pos)
{
	if (parent())
		return parent()->fromRootPos(pos) - geometry().contentBox().pos();
	else
		return pos;
}

const WindowFrame* Element::window() const
{
	Element* super = parent();
	if (super)
		return super->window();
	else
		return viewwindow;
}

WindowFrame* Element::window()
{
	Element* super = parent();
	if (super)
		return super->window();
	else
		return viewwindow;
}

void Element::render(Canvas* canvas)
{
	renderStyle(canvas);
	Point origin = canvas->getOrigin();
	canvas->setOrigin(origin + geometry().contentBox().pos());
	renderContent(canvas);
	canvas->setOrigin(origin);
}

Colorf Element::color()
{
	if (parent())
		return parent()->color();
	else
		return { 0.0f, 0.0f, 0.0f };
}

double Element::lineHeight()
{
	if (isClass("tabbartab-label"))
		return 20;
	else if (isClass("toolbar"))
		return 30;
	else if (isClass("menubar") || isClass("menubarmodal"))
		return 30;
	else if (isClass("menubaritem") || isClass("menubarmodalitem"))
		return 24;
	else if (isClass("listviewheader"))
		return 24;
	else if (isClass("listviewitem"))
		return 24;

	if (parent())
		return parent()->lineHeight();
	else
		return 20;
}

ComputedBorder Element::computedBorder()
{
	ComputedBorder border;
	if (isClass("tabbartab"))
	{
		border.top = 6;
		border.bottom = 6;
		border.left = 15;
		border.right = 15;
	}
	else if (isClass("toolbarbutton"))
	{
		border.right = 10;
	}
	else if (isClass("toolbar"))
	{
		border.top = 5;
		border.right = 5;
		border.bottom = 5;
		border.left = 10;
	}
	else if (isClass("menubar") || isClass("menubarmodal"))
	{
		border.left = 8;
		border.right = 8;
	}
	else if (isClass("menubaritem") || isClass("menubarmodalitem"))
	{
		border.top = 3;
		border.bottom = 3;
		border.left = 9;
		border.right = 9;
	}
	else if (isClass("listviewheader"))
	{
		border.left = 5;
		border.right = 5;
	}
	else if (isClass("listviewitem"))
	{
		border.left = 5;
		border.right = 5;
	}
	else if (parent() && parent()->isClass("listview-headersplitter"))
	{
		border.left = 1;
	}
	return border;
}

void Element::renderStyle(Canvas* canvas)
{
	if (isClass("debuggerwindow"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(240 / 255.0f, 240 / 255.0f, 240 / 255.0f));
	}
	else if (isClass("tabbartab") && isClass("selected"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(255 / 255.0f, 255 / 255.0f, 255 / 255.0f));
		canvas->fillRect(geometry().contentBox(), Colorf(255 / 255.0f, 255 / 255.0f, 255 / 255.0f));
	}
	else if (isClass("tabcontrol-widgetstack"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(255 / 255.0f, 255 / 255.0f, 255 / 255.0f));
	}
	else if (isClass("listviewheader"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(240 / 255.0f, 240 / 255.0f, 240 / 255.0f));
	}
	else if (isClass("listviewbody"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(255 / 255.0f, 255 / 255.0f, 255 / 255.0f));
	}
	else if (isClass("listviewitem") && isClass("selected"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(204 / 255.0f, 232 / 255.0f, 255 / 255.0f));
	}
	else if (parent() && parent()->isClass("listview-headersplitter"))
	{
		canvas->fillRect(geometry().paddingBox(), Colorf(200 / 255.0f, 200 / 255.0f, 200 / 255.0f));
	}
}

void Element::setParent(Element* newParent)
{
	if (parentObj != newParent)
	{
		if (parentObj)
			detachFromParent();

		if (newParent)
		{
			prevSiblingObj = newParent->lastChildObj;
			if (prevSiblingObj) prevSiblingObj->nextSiblingObj = this;
			newParent->lastChildObj = this;
			if (!newParent->firstChildObj) newParent->firstChildObj = this;
			parentObj = newParent;
		}
	}
}

void Element::moveBefore(Element* sibling)
{
	if (sibling && sibling->parentObj != parentObj) throw std::runtime_error("Invalid sibling passed to Element.moveBefore");
	if (!parentObj) throw std::runtime_error("View must have a parent before it can be moved");

	if (nextSiblingObj != sibling)
	{
		Element* p = parentObj;
		detachFromParent();

		parentObj = p;
		if (sibling)
		{
			nextSiblingObj = sibling;
			prevSiblingObj = sibling->prevSiblingObj;
			sibling->prevSiblingObj = this;
			if (prevSiblingObj) prevSiblingObj->nextSiblingObj = this;
			if (parentObj->firstChildObj == sibling) parentObj->firstChildObj = this;
		}
		else
		{
			prevSiblingObj = parentObj->lastChildObj;
			if (prevSiblingObj) prevSiblingObj->nextSiblingObj = this;
			parentObj->lastChildObj = this;
			if (!parentObj->firstChildObj) parentObj->firstChildObj = this;
		}
	}
}

void Element::detachFromParent()
{
	if (prevSiblingObj)
		prevSiblingObj->nextSiblingObj = nextSiblingObj;
	if (nextSiblingObj)
		nextSiblingObj->prevSiblingObj = prevSiblingObj;
	if (parentObj)
	{
		if (parentObj->firstChildObj == this)
			parentObj->firstChildObj = nextSiblingObj;
		if (parentObj->lastChildObj == this)
			parentObj->lastChildObj = prevSiblingObj;
	}
	prevSiblingObj = nullptr;
	nextSiblingObj = nullptr;
	parentObj = nullptr;
}

double Element::preferredWidth(Canvas* canvas)
{
	if (fixedWidth >= 0)
	{
		return fixedWidth;
	}
	else if (!innerText.empty())
	{
		return canvas->measureText(innerText).width;
	}
	else if (isClass("vbox"))
	{
		double w = 0.0;
		for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
		{
			ComputedBorder border = element->computedBorder();
			w = std::max(element->preferredWidth(canvas) + border.left + border.right, w);
		}
		return w;
	}
	else
	{
		double w = 0.0;
		for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
		{
			ComputedBorder border = element->computedBorder();
			w += element->preferredWidth(canvas) + border.left + border.right;
		}
		return w;
	}
}

double Element::preferredHeight(Canvas* canvas, double width)
{
	if (fixedHeight >= 0)
	{
		return fixedHeight;
	}
	else if (!innerText.empty())
	{
		return lineHeight();
	}
	else if (isClass("vbox"))
	{
		double h = 0.0;
		for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
		{
			ComputedBorder border = element->computedBorder();
			h += element->preferredHeight(canvas, width - border.left - border.right) + border.top + border.bottom;
		}
		return h;
	}
	else
	{
		double h = 0.0;
		for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
		{
			ComputedBorder border = element->computedBorder();
			h = std::max(element->preferredHeight(canvas, width) + border.top + border.bottom, h);
		}
		return h;
	}
}

double Element::firstBaselineOffset(Canvas* canvas, double width)
{
	if (isClass("vbox"))
	{
		if (firstChild())
		{
			ComputedBorder border = firstChild()->computedBorder();
			return firstChild()->firstBaselineOffset(canvas, width) + border.top;
		}
		else
		{
			return 0.0f;
		}
	}
	else
	{
		if (firstChild())
			return firstChild()->firstBaselineOffset(canvas, width);
		else
			return 0.0f;
	}
}

double Element::lastBaselineOffset(Canvas* canvas, double width)
{
	if (isClass("vbox"))
	{
		if (firstChild())
		{
			double h = 0.0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				if (element != lastChild())
				{
					ComputedBorder border = element->computedBorder();
					h += element->preferredHeight(canvas, width) + border.top + border.bottom;
				}
			}
			return h + lastChild()->lastBaselineOffset(canvas, width);
		}
		else
		{
			return 0.0f;
		}
	}
	else
	{
		if (firstChild())
		{
			double h = 0.0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				if (element != lastChild())
					h += element->preferredHeight(canvas, width);
			}
			return h + lastChild()->lastBaselineOffset(canvas, width);
		}
		else
		{
			return 0.0f;
		}
	}
}

void Element::renderContent(Canvas* canvas)
{
	if (needsLayout())
	{
		if (isClass("vbox"))
		{
			double width = geometry().contentWidth;

			double totalheight = 0;
			int expandingcount = 0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				ComputedBorder border = element->computedBorder();
				totalheight += element->preferredHeight(canvas, width - border.left - border.right) + border.top + border.bottom;
				if (element->isClass("expanding"))
					expandingcount++;
			}
			double stretchheight = expandingcount > 0 ? (geometry().contentHeight - totalheight) / expandingcount : 0.0;

			double y = 0.0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				ComputedBorder border = element->computedBorder();

				ElementGeometry childpos;
				childpos.paddingLeft = border.left;
				childpos.paddingTop = border.top;
				childpos.paddingRight = border.right;
				childpos.paddingBottom = border.bottom;
				childpos.contentX = border.left;
				childpos.contentY = y + childpos.paddingTop;
				childpos.contentWidth = width - border.left - border.right;
				childpos.contentHeight = element->preferredHeight(canvas, childpos.contentWidth);
				if (element->isClass("expanding"))
					childpos.contentHeight = std::max(childpos.contentHeight + stretchheight, 0.0);
				element->setGeometry(childpos);

				y += childpos.contentHeight + childpos.paddingTop + childpos.paddingBottom;
			}
		}
		else
		{
			double height = geometry().contentHeight;

			double totalwidth = 0;
			int expandingcount = 0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				ComputedBorder border = element->computedBorder();
				totalwidth += element->preferredWidth(canvas) + border.left + border.right;
				if (element->isClass("expanding"))
					expandingcount++;
			}
			double stretchwidth = expandingcount > 0 ? (geometry().contentWidth - totalwidth) / expandingcount : 0.0;

			double x = 0.0;
			for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
			{
				ComputedBorder border = element->computedBorder();

				ElementGeometry childpos;
				childpos.paddingLeft = border.left;
				childpos.paddingTop = border.top;
				childpos.paddingRight = border.right;
				childpos.paddingBottom = border.bottom;
				childpos.contentX = x + border.left;
				childpos.contentY = border.top;
				childpos.contentWidth = element->preferredWidth(canvas);
				childpos.contentHeight = height - border.top - border.bottom;
				if (element->isClass("expanding"))
					childpos.contentWidth = std::max(childpos.contentWidth + stretchwidth, 0.0);
				element->setGeometry(childpos);

				x += childpos.contentWidth + childpos.paddingLeft + childpos.paddingRight;
			}
		}
		clearNeedsLayout();
	}

	for (Element* element = firstChild(); element != nullptr; element = element->nextSibling())
	{
		element->render(canvas);
	}

	if (!innerText.empty())
	{
		canvas->drawText({ 0.0f, 0.0f }, color(), innerText);
	}
}