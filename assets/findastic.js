(function($) {

  // Translate filter.
  Twig.filters.t = function(text) {
    var attributes = ['class="findastic-placeholder"'];
    attributes.push('data-function="t"');
    attributes.push('data-args_0="' + text + '"');
    return '<span ' + attributes.join(' ') +  '></span>';
  };

  // Entity field filter.
  Twig.filters.entity_view_field = function(entity_id, args) {
    var attributes = ['class="findastic-placeholder"'];
    attributes.push('data-function="findastic_entity_view_field"');
    attributes.push('data-args_0="' + args[0] + '"');
    attributes.push('data-args_1="' + entity_id + '"');
    attributes.push('data-args_2="' + args[1] + '"');
    return '<span ' + attributes.join(' ') +  '></span>';
  };

  // Build findastic object.
  Drupal.findastic = Drupal.findastic || {};
  Drupal.findastic.ajax = false;

  // Define findasti request function.
  Drupal.findastic.request = function(url, options) {
    // Abort previous ajax request.
    if (Drupal.findastic.ajax) {
      Drupal.findastic.ajax.abort();
    }

    // Trackpageview in google analytics.
    if (typeof ga !== 'undefined') {
      ga('send', 'pageview', url);
    }

    // Prepare ajax options.
    var ajax_options = $.extend({
      url: url,
      dataType: "json",
      cache: false
    }, options);

    // Perform new ajax.
    Drupal.findastic.ajax = $.ajax(ajax_options);
  };

  // Bind the StateChange Event
  $(window).bind('history:statechange', function() {
    // Perform findastic request call.
    Drupal.findastic.request(History.getState(), {
      beforeSend: function() {
        $('#findastic-wrapper').addClass('loading');
        $('.findastic-facet').addClass('loading');
      },
      complete: function() {
        Drupal.findastic.ajax = false;
        $('#findastic-wrapper').removeClass('loading');
        $('.findastic-facet').removeClass('loading');
      },
      success: function(response) {
        // Add settings if exist.
        if (typeof response.settings !== 'undefined') {
          $.extend(true, Drupal.settings, response.settings);
        }

        // Append page results.
        $('#findastic-wrapper').html(
          Twig.twig({
            'data': Drupal.settings.findastic.template
          }).render(response.data)
        );

        // Attach Behaviors.
        Drupal.attachBehaviors($('#findastic-wrapper'));

        // Trigger update in facets components.
        Event.$emit('facets.update', response.data.facets);

        // Update sliders.
        if (typeof $.fn.rangeSlider !== 'undefined') {
          $('.findastic-slider').each(function() {
            $this = $(this);
            field_id = $this.attr('id').replace('findastic-slider-', '');
            $this.children('.findastic-slider-range').rangeSlider('update', response.data.facets[field_id] || []);
          });
        }
      }
    });
  });

  Drupal.behaviors.findastic = {
    attach: function(context) {
      var $context = context;

      if ($('.findastic-placeholder:not([rel])', context).length > 0) {
        var data = {};
        var unique_id = new Date().getTime() + '' + Math.round(Math.random() * 10000);
        $('.findastic-placeholder:not([rel])', context).each(function() {
          var $this = $(this);
          var func = $this.data('function');
          if (typeof data[func] === 'undefined') {
            data[func] = [];
          }
          var item_data = {};
          $.each($this.data(), function(key, value) {
            if (key.indexOf('args') > -1) {
              item_data[key.replace('args_', '')] = value;
            }
          });
          data[func].push(item_data);
        }).attr('rel', unique_id);

        // Get the current path with query parameters.
        var url = History.URL();
        var path = url.path + (url.query.toString() && ('?' + url.query));

        // For instant search get the instant search path.
        if (History.getHash().indexOf('search') === 0) {
          var path = '/findastic?search=' + History.unescapeHash(History.getHash().split('/').pop());
        }

        $.ajax({
          url: Drupal.settings.basePath + Drupal.settings.pathPrefix + 'ajax/findastic?rel=' + unique_id + '&path=' + encodeURIComponent(path),
          data: {placeholder: data},
          dataType: 'json',
          method: 'POST',
          success: function(response) {
            // Add settings if exist.
            if (typeof response.settings !== 'undefined') {
              $.extend(true, Drupal.settings, response.settings);
            }

            $('.findastic-placeholder[rel="' + response.rel + '"]').each(function(index) {
              $(this).replaceWith(response.replacements[index]);
            });
            Drupal.attachBehaviors($('#findastic-wrapper'));
          }
        });
      }

      // Render findastic facets with VueJS.
      // We use setTimeout to make it async.
      setTimeout(function() {
        $('.findastic-facet:not(".findastic-search-facet")', $context).once(function() {
          if (Drupal.settings.findastic.__INITIAL_STATE__) {
            var $this = $(this);
            new Facet({
              propsData: {
                id: $this.data('id'),
                name: $this.data('name'),
                facets: Drupal.settings.findastic.__INITIAL_STATE__.facets['_' + $this.data('id')]
              }
            }).$mount($(this)[0]);
          }
        });
      }, 0);

      // Add custom range widget.
      $('.findastic-custom-range', context).customRange();

      // Pagination.
      $('.findastic-filters a', context).bind('click', function(e) {
        e.preventDefault();
        History.state($(this).attr('href'));
      });

      // Pagination.
      $('.findastic-pager a', context).bind('click', function(e) {
        e.preventDefault();
        History.state($(this).attr('href'));
        if (Drupal.settings.findastic.scroll_to_top) {
          $('html, body').stop(true, false).animate({
            scrollTop: $('#findastic-wrapper').offset().top
          }, 300, "linear");
        }
      });

      $('select.findastic-sort, .findastic-items-per-page', context).bind('change', function() {
        var url  = History.URL();
        var name = $(this).attr('name');
        url.query[name] = this.value;
        if (url.query['page'] !== undefined) {
          delete url.query['page'];
        }
        History.state(url.toString());
      });
    }
  };

  // Custom jQuery widget for custom range.
  $.widget("findastic.customRange", {
    options: {
      name: '',
      defaultMin: 0,
      defaultMax: 10,
      values: [0, 10]
    },
    _create: function() {
      var self = this;

      this.options.name = this.element.data('name');
      this.options.defaultMin = $('.custom-range-min', this.element).val();
      this.options.defaultMax = $('.custom-range-max', this.element).val();
      this._value(0, this.options.defaultMin);
      this._value(1, this.options.defaultMax);
      this._refresh();

      $('.custom-range-min', this.element).bind('keyup', function() {
        self.value(0, this.value);
      });

      $('.custom-range-max', this.element).bind('keyup', function() {
        self.value(1, this.value);
      });

      $(this.element).bind('submit', function(e) {
        e.preventDefault();
        var url  = History.URL();
        url.query['min-' + self.options.name] = self.options.values[0];
        url.query['max-' + self.options.name] = self.options.values[1];
        History.state(url.toString());
        self.options.defaultMin = self.options.values[0];
        self.options.defaultMax = self.options.values[1];
        self._refresh();
      });
    },
    values: function(values) {
      $('.custom-range-min', this.element).val(values[0]);
      $('.custom-range-max', this.element).val(values[1]);
      this.options.values = values;
      this.options.defaultMin = values[0];
      this.options.defaultMax = values[1];
      this._refresh();
    },
    value: function(index, value) {
      this._value(index, value);
      this._refresh();
    },
    _value: function(index, value) {
      this.options.values[index] = value;
    },
    _refresh: function() {
      if (this.options.values[0] == this.options.defaultMin && this.options.values[1] == this.options.defaultMax) {
        $('.form-submit', this.element).attr('disabled', 'disabled');
      }
      else {
        $('.form-submit', this.element).removeAttr('disabled');
      }
    }
  });

  // VueJS implementation of facets.
  var Event = new Vue();

  // Facet Links component.
  var Facet = Vue.extend({
    props: ['id', 'name', 'facets'],
    template: '\
      <ul :class="classes" :id="html_id">\
        <facet-link v-for="(item, key) in items" :item="item"></facet-link>\
      </ul>\
    ',
    data: function() {
      return {
        items: this.facets
      };
    },
    mounted: function() {
      var self = this;
      this.$listener = Event.$on('facets.update', function(newfacets) {
        self.items = newfacets[self.id];
      });
    },
    computed: {
      html_id: function() {
        return 'findastic-facet-' + this.id;
      },
      classes: function() {
        return [
          'findastic-facet',
          'findastic-facet-' + this.id,
          'findastic-facet-' + this.name
        ];
      }
    }
  });

  // Facet link component.
  Vue.component('facet-link', {
    props: ['item'],
    template: '\
      <li :class="term">\
        <a :rel="rel" :class="{\'active\': active, \'checked\': checked}" :href="url" @click="click($event)"><label v-html="title"></label><span> ({{ num }})</span></a>\
      </li>\
    ',
    methods: {
      click: function(e) {
        e.preventDefault();

        if (this.active) {
          this.checked = !this.checked;
          History.state(this.url);
        }
      }
    },
    computed: {
      title: function() {
        return this.item.t;
      },
      url: function() {
        return this.item.u;
      },
      num: function() {
        return this.item.n;
      },
      checked: function() {
        return this.item.c;
      },
      active: function() {
        return this.num > 0;
      },
      term: function() {
        return this.item.i;
      },
      rel: function() {
        return this.item.r;
      }
    }
  });

})(jQuery);
