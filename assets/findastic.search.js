var SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition,
    SpeechGrammarList = window.SpeechGrammarList || window.webkitSpeechGrammarList,
    SpeechRecognitionEvent = window.SpeechRecognitionEvent || window.webkitSpeechRecognitionEvent;

(function ($) {

  // Bind the StateChange Event
  $(window).bind('history:hashchange', function() {
    if (History.getHash().indexOf('search') === 0) {
      $('.findastic-overlay-search:first').overlaySearch('search', History.getHash());
    }
    else {
      $('.findastic-overlay-search').overlaySearch('close');
    }
  });

  Drupal.behaviors.findasticSearch = {
    attach: function (context, settings) {
      if (settings.findastic) {
        // Add search widget.
        $('.findastic-search', context).search(settings.findastic.search);

        // Add overlay search widget.
        $('.findastic-overlay-search', context).overlaySearch({
          path: settings.findastic.search_path
        });

        if (Drupal.settings.findastic.voice_search_enabled) {
          $('body').once(function(){
            if (typeof window.SpeechRecognition !== 'undefined') {
              $(this).addClass('findastic-speech-enabled');
            }
          });

          $('.findastic-external-speech-recognition-button', context).once(function(){
            $(this).on('click', function(e){
              e.preventDefault();

              var parent = $(this).parent(),
                  search = $('a.findastic-overlay-search', parent),
                  uuid = search.data('uuid');

              search.trigger('click');
              $('.findastic-speech-recognition-button', '#findastic-overlay-search-wrapper-' + uuid).trigger('click');
            });
          });

          $('.findastic-speech-recognition-button', context).once(function() {
            var recognition = null,
                $this = $(this),
                overlayElement = $this.parents('.findastic-overlay-search-wrapper');

            $(document).on('keyup', function(e){
              if (e.keyCode == 27) { // escape key maps to keycode `27`
                if (recognition instanceof SpeechRecognition) {
                  recognition.abort();
                  $('#findastic-search-input', overlayElement).val('');
                }
              }
            });

            $('.findastic-overlay-search-close', overlayElement).on('click', function(){
              if (recognition instanceof SpeechRecognition) {
                recognition.abort();
                $('#findastic-search-input', overlayElement).val('');
              }
            });

            $(this).on('click', function(e) {
              e.preventDefault();

              if (typeof window.SpeechRecognition === 'undefined') {
                return false;
              }

              $('#findastic-search-input', overlayElement).val('');

              if (window.ga) {
                ga('send', {
                  hitType: 'event',
                  eventCategory: 'Speech',
                  eventAction: 'click'
                });
              }

              recognition = new SpeechRecognition();
              recognition.lang = 'el-GR';

              recognition.onstart = function() {
                $this.addClass('listening');
              };

              recognition.onend = function() {
                $this.removeClass('listening');
              };

              recognition.onresult = function(event) {
                var transcript = '';
                for (var i = event.resultIndex; i < event.results.length; ++i) {
                  if (event.results[i].isFinal) {
                    transcript += event.results[i][0].transcript;
                  }
                }

                if (transcript !== '') {
                  $('#findastic-search-input', overlayElement).val(transcript).trigger('keyup');

                  if (window.ga) {
                    ga('send', {
                      hitType: 'event',
                      eventCategory: 'Speech',
                      eventAction: 'result',
                      eventLabel: transcript
                    });
                  }
                }
              };

              recognition.start();
            });
          });
        }
      }
    }
  };

  // Custom jQuery widget for findastic search.
  $.widget("findastic.search", {
    options: {
      argument: false,
      path: '/search/products',
    },
    _create: function() {
      var self = this;

      // Bind form submit.
      this.element.bind('submit', function(e) {
        e.preventDefault();
        var text = $('input:text', $(this)).val();
        self._search(text);
      });

      // Bind instant search.
      $('input:text.instant', this.element).bind('keyup', function(e) {
        e.preventDefault();
        self._search(this.value);
      });
    },
    _search: function(value) {
      if (this.options.argument) {
        if (value == '') {
          History.state(this.options.path);
        }
        else {
          History.state(this.options.path + '/' + value);
        }
      }
      else {
        var url  = History.URL();

        if (value == '') {
          delete url.query['search'];
        }
        else {
          url.query['search'] = value;
        }

        History.state(url.toString());
      }
    }
  });

  // Custom jQuery widget for findastic overlay search.
  $.widget("findastic.overlaySearch", {
    options: {
      path: '/search/products',
    },
    _create: function() {
      var self = this;

      // Open overlay search.
      this.element.bind('click', function(e) {
        e.preventDefault();
        History.hash('search');
      });

      // Add the uuid as a data attribute in order to be able to
      // link the search button to the search wrapper.
      this.element.attr('data-uuid', this.uuid);

      // Append overlay wrapper to body.
      this.overlayElement = $('<div class="findastic-overlay-search-wrapper" id="findastic-overlay-search-wrapper-' + this.uuid + '"></div>');
      this.overlayElement.hide().appendTo('body');
      this.overlayElement.html(Drupal.settings.findastic.overlay_markup);

      // Close button.
      $('a.findastic-overlay-search-close', this.overlayElement).bind('click', function(e) {
        e.preventDefault();
        e.stopPropagation();
        self.close();
      });

      // Handle input keup.
      $('input:text', this.overlayElement).bind('keyup', function(e) {
        e.preventDefault();
        e.stopPropagation();

        // On escape button trigger close.
        if (e.keyCode === 27) {
          self.close();
          return;
        }

        // Update the hash on browser.
        History.hash('search/' + History.escapeHash(this.value));
      });

      $(this.overlayElement).bind('scroll', function(e) {
        if ($('.findastic-overlay-pager', self.overlayElement).length > 0) {
          if ($('.findastic-overlay-search-container', self.overlayElement).height() - 700 < $(self.overlayElement).scrollTop() + $(window).height()) {
            var url = $('.findastic-overlay-pager [class*="next"] a', self.overlayElement).attr('href');
            $('.findastic-overlay-pager', self.overlayElement).remove();
            if (url) {
              $.ajax({
                url: url,
                dataType: "json",
                cache: false,
                success: function(response) {
                  var $content = $('<div>' + response.content + '</div>');
                  var $context = $('.findastic-overlay-search-results', self.overlayElement);
                  var $pager = $content.find('.findastic-pager:first');
                  $context.append($content.find('.findastic-content').html());
                  if ($pager.length) {
                    $context.append('<div class="findastic-overlay-pager">' + $pager.html() + '</div>');
                  }
                  if (typeof response.settings !== 'undefined') {
                    $.extend(true, Drupal.settings, response.settings);
                  }
                  Drupal.attachBehaviors($context, Drupal.settings);
                }
              });
            }
          }
        }
      });
    },
    search: function(hash) {
      var self = this;

      // Trigger open.
      self.open();

      // Add the value from url to the input.
      if (hash.indexOf('/') > -1) {
        var value = History.unescapeHash(hash.split('/').pop());
        var $input = $('input:text', this.overlayElement);
        if ($input.val() != value) {
          $input.val(value);
        }

        // Perform "non-blocking" ajax call.
        if (value !== '') {
          // Trackpageview in google analytics.
          if (typeof ga !== 'undefined') {
            ga('send', 'pageview', Drupal.settings.basePath + Drupal.settings.pathPrefix + '?search=' + History.escapeHash(value));
          }

          $.ajax({
            url: Drupal.settings.basePath + Drupal.settings.pathPrefix + 'instant-search?text=' + History.escapeHash(value),
            dataType: "json",
            cache: false,
            success: function(response) {
              var $content = $('<div>' + response.content + '</div>');
              var $context = $('.findastic-overlay-search-results', self.overlayElement);
              var $pager = $content.find('.findastic-pager:first');
              var $markup = ($content.find('div.messages').length > 0) ? $content.find('div.messages').html() : '';
              $markup += $content.find('.findastic-content').html();
              $context.html($markup);
              if ($pager.length) {
                $context.append('<div class="findastic-overlay-pager">' + $pager.html() + '</div>');
              }
              if (typeof response.settings !== 'undefined') {
                $.extend(true, Drupal.settings, response.settings);
              }
              Drupal.attachBehaviors($context, Drupal.settings);
            }
          });
        }
        else {
          $('.findastic-overlay-search-results', self.overlayElement).html('');
        }
      }
    },
    open: function() {
      if (this.overlayElement.is(':visible')) {
        return;
      }

      $('html, body').animate({scrollTop: 0}, 200, 'easeOutExpo');

      // Display search overlay.
      $('body').addClass('findastic-overlay-search--opening');
      this.overlayElement.fadeIn(400, function() {
        $('body').addClass('findastic-overlay-search--open');
        $('body').removeClass('findastic-overlay-search--opening');
        // Focus textfield.
        $('input:text', $(this)).trigger('focus');
      });
    },
    close: function() {
      if (this.overlayElement.is(':hidden')) {
        return;
      }
      History.hash('');
      $('body').addClass('findastic-overlay-search--closing');
      this.overlayElement.fadeOut(400, function(){
        $('body').removeClass('findastic-overlay-search--closing');
        $('body').removeClass('findastic-overlay-search--open');
      });
    }
  });

})(jQuery);
