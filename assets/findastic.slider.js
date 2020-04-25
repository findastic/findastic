(function($) {

  Drupal.behaviors.findasticSlider = {
    attach: function(context) {
      // Initialize findastic sliders.
      $('.findastic-slider', context).once(function() {
        $('.findastic-slider-range', $(this)).rangeSlider();
      });
    }
  };

  // Custom jQuery widget that extends $.ui.slider.
  $.widget("findastic.rangeSlider", $.ui.slider, {
    options: {
      id: '',
      name: '',
      logarithmic: false,
      value: null,
      values: [0, 100],
      range: true,
      animate: true
    },
    update: function(data) {
      this.options.min = data.min;
      this.options.max = data.max;
      data.values[0] = this._expslider(data.values[0]);
      data.values[1] = this._expslider(data.values[1]);
      this.values(data.values);
      this._refreshVisibility();
      this._refreshSlideTexts();
    },
    _create: function() {
      self = this;

      // Create default values from data.
      $.each(this.element.data(), function(key, value) {
        if (key.indexOf('slider') == 0) {
          var optionName = (key.replace('slider', '')).toLowerCase();
          self.options[optionName] = value;
        }
      });

      // Apply logarithmic calculation if isset.
      this.options.values[0] = this._expslider(this.options.values[0]);
      this.options.values[1] = this._expslider(this.options.values[1]);

      return this._super();
    },
    _slide: function(event, index, newVal) {
      this._super(event, index, newVal);
      this._updateSlideText(index, newVal);
    },
    _stop: function(event, index) {
      this._super(event, index);

      var values = this.values();
      var prefix = index == 0 ? 'min-' : 'max-';
      var url  = History.URL();
      url.query[prefix + this.options.name] = this._logslider(values[index]);
      if (url.query['page'] !== undefined) {
        delete url.query['page'];
      }
      History.state(url.toString());
    },
    // Helper function for logarithic calculation.
    _logslider: function(Lvalue) {
      if (!this.options.logarithmic) return Lvalue;
      if (Lvalue === 0) return 0;
      var Lminv = Math.log(this.options.min);
      var Lmaxv = Math.log(this.options.max);
      Lscale = (Lmaxv - Lminv) / (this.options.max - this.options.min);
      return Math.round(Math.exp(Lminv + Lscale * (Lvalue - this.options.min)));
    },
    // Helper function for logarithic calculation.
    _expslider: function(Lvalue) {
      if (!this.options.logarithmic) return Lvalue;
      if (Lvalue === 0) return 0;
      Lminv = Math.log(this.options.min);
      Lmaxv = Math.log(this.options.max);
      Lscale = (Lmaxv - Lminv) / (this.options.max - this.options.min);
      Lnum = Math.log(Lvalue);
      return Math.round(((Lnum - Lminv) / Lscale) + this.options.min);
    },
    // Helper function for updating findastic slide text.
    _updateSlideText: function(index, newVal) {
      value = this._logslider(newVal);
      position = (newVal - this.options.min) / (this.options.max - this.options.min) * 100;
      selector = index == 0 ? ".findastic-slider-from" : ".findastic-slider-to";
      $(selector, this.element)
        .css('left', position + '%')
        .addClass('picked')
        .children('strong')
        .html(value.format(this.options.decimals, this.options.decimalsep, this.options.thousandsep));

      // Also update custom range if isset.
      $('#findastic-custom-range-form-' + this.options.id).customRange('values', this.options.values);
    },
    // Refresh functions.
    _refresh: function() {
      this._super();
      this._refreshVisibility();
      this._refreshSlideTexts();
    },
    _refreshSlideTexts: function() {
      this._updateSlideText(0, this.options.values[0]);
      this._updateSlideText(1, this.options.values[1]);
    },
    _refreshVisibility: function() {
      if (this.options.min == this.options.max) {
        this.element.closest('.block').hide();
        this.disable();
      }
      else {
        this.enable();
        this.element.closest('.block').show();
      }
    },
  });

  // Allow numbers to be formatted.
  Number.prototype.format = function(decimals, decPoint, thousandsSep) {
    number = (this + '').replace(/[^0-9+\-Ee.]/g, '')
    var n = !isFinite(+number) ? 0 : +number
    var prec = !isFinite(+decimals) ? 0 : Math.abs(decimals)
    var sep = (typeof thousandsSep === 'undefined') ? ',' : thousandsSep
    var dec = (typeof decPoint === 'undefined') ? '.' : decPoint
    var s = ''

    var toFixedFix = function (n, prec) {
      var k = Math.pow(10, prec)
      return '' + (Math.round(n * k) / k)
        .toFixed(prec)
    }

    // @todo: for IE parseFloat(0.55).toFixed(0) = 0;
    s = (prec ? toFixedFix(n, prec) : '' + Math.round(n)).split('.')
    if (s[0].length > 3) {
      s[0] = s[0].replace(/\B(?=(?:\d{3})+(?!\d))/g, sep)
    }
    if ((s[1] || '').length < prec) {
      s[1] = s[1] || ''
      s[1] += new Array(prec - s[1].length + 1).join('0')
    }

    return s.join(dec);
  };

}) (jQuery);
